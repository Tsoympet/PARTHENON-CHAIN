#include "wallet.h"

#include <algorithm>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <stdexcept>

namespace wallet {

static PubKey Compress(const std::array<uint8_t, 32>& x)
{
    PubKey out{};
    out[0] = 0x02;
    std::copy(x.begin(), x.end(), out.begin() + 1);
    return out;
}

WalletBackend::WalletBackend(KeyStore store)
    : m_store(std::move(store)) {}

KeyId WalletBackend::ImportKey(const PrivKey& priv)
{
    KeyId id{};
    SHA256(priv.data(), priv.size(), id.data());
    m_store.Import(id, priv);
    return id;
}

bool WalletBackend::GetKey(const KeyId& id, PrivKey& out) const
{
    return m_store.Get(id, out);
}

void WalletBackend::AddUTXO(const OutPoint& op, const TxOut& txout)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_utxos.push_back({op, txout});
}

void WalletBackend::SetUTXOLookup(UTXOLookup lookup)
{
    m_lookup = std::move(lookup);
}

void WalletBackend::SyncFromLayer1(const std::vector<OutPoint>& watchlist)
{
    if (!m_lookup) return;
    std::lock_guard<std::mutex> g(m_mutex);
    for (const auto& op : watchlist) {
        auto existing = std::find_if(m_utxos.begin(), m_utxos.end(), [&op](const UTXO& u) {
            return u.outpoint.hash == op.hash && u.outpoint.index == op.index;
        });
        if (existing != m_utxos.end()) continue;
        auto maybe = m_lookup(op);
        if (maybe) {
            m_utxos.push_back({op, *maybe});
        }
    }
}

uint64_t WalletBackend::GetBalance() const
{
    std::lock_guard<std::mutex> g(m_mutex);
    uint64_t total = 0;
    for (const auto& u : m_utxos) total += u.txout.value;
    return total;
}

std::vector<UTXO> WalletBackend::SelectCoins(uint64_t amount) const
{
    std::vector<UTXO> chosen;
    uint64_t acc = 0;
    for (const auto& u : m_utxos) {
        chosen.push_back(u);
        acc += u.txout.value;
        if (acc >= amount) break;
    }
    if (acc < amount) throw std::runtime_error("insufficient funds");
    return chosen;
}

PubKey WalletBackend::DerivePub(const PrivKey& priv) const
{
    std::array<uint8_t, 32> hashed{};
    SHA256(priv.data(), priv.size(), hashed.data());
    return Compress(hashed);
}

std::vector<uint8_t> WalletBackend::SignDigest(const PrivKey& key, const Transaction& tx, size_t inputIndex) const
{
    auto ser = Serialize(tx);
    std::vector<uint8_t> digest(32);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, ser.data(), ser.size());
    SHA256_Update(&ctx, &inputIndex, sizeof(inputIndex));
    SHA256_Final(digest.data(), &ctx);

    unsigned int len = 0;
    std::vector<uint8_t> sig(EVP_MAX_MD_SIZE);
    HMAC(EVP_sha256(), key.data(), key.size(), digest.data(), digest.size(), sig.data(), &len);
    sig.resize(len);
    return sig;
}

Transaction WalletBackend::CreateSpend(const std::vector<TxOut>& outputs, const KeyId& from, uint64_t fee)
{
    PrivKey key;
    if (!m_store.Get(from, key)) throw std::runtime_error("missing key");
    uint64_t value = fee;
    for (const auto& o : outputs) value += o.value;
    auto coins = SelectCoins(value);

    Transaction tx;
    tx.vout = outputs;
    uint64_t inTotal = 0;
    for (const auto& c : coins) {
        TxIn in;
        in.prevout = c.outpoint;
        in.sequence = 0xffffffff;
        tx.vin.push_back(in);
        inTotal += c.txout.value;
    }
    if (inTotal > value) {
        TxOut change{inTotal - value, {0x6a}}; // OP_RETURN placeholder for change script
        tx.vout.push_back(change);
    }

    for (size_t i = 0; i < tx.vin.size(); ++i) {
        tx.vin[i].scriptSig = SignDigest(key, tx, i);
    }
    std::vector<OutPoint> spent;
    spent.reserve(tx.vin.size());
    for (const auto& in : tx.vin) spent.push_back(in.prevout);
    RemoveCoins(spent);
    return tx;
}

void WalletBackend::SetHDSeed(const std::array<uint8_t, 32>& seed)
{
    unsigned int len = 0;
    std::array<uint8_t, 64> I{};
    HMAC(EVP_sha512(), "DRACHMA seed", 11, seed.data(), seed.size(), I.data(), &len);
    std::copy(I.begin(), I.begin() + 32, m_master.priv.begin());
    std::copy(I.begin() + 32, I.begin() + 64, m_master.chainCode.begin());
    m_master.pub = DerivePub(m_master.priv);
    m_master.depth = 0;
    m_master.childNumber = 0;
    m_hasSeed = true;
}

HDNode WalletBackend::DeriveChild(uint32_t index)
{
    if (!m_hasSeed) throw std::runtime_error("missing seed");
    std::array<uint8_t, 4> idxBytes{static_cast<uint8_t>(index >> 24), static_cast<uint8_t>(index >> 16), static_cast<uint8_t>(index >> 8), static_cast<uint8_t>(index)};
    unsigned int len = 0;
    std::array<uint8_t, 64> out{};
    HMAC(EVP_sha512(), m_master.chainCode.data(), m_master.chainCode.size(), idxBytes.data(), idxBytes.size(), out.data(), &len);
    HDNode child;
    std::copy(out.begin(), out.begin() + 32, child.priv.begin());
    std::copy(out.begin() + 32, out.begin() + 64, child.chainCode.begin());
    child.depth = m_master.depth + 1;
    child.childNumber = index;
    child.pub = DerivePub(child.priv);
    return child;
}

std::vector<uint8_t> WalletBackend::BuildMultisigScript(const std::vector<PubKey>& pubs, uint8_t m) const
{
    std::vector<uint8_t> script;
    script.push_back(static_cast<uint8_t>(0x50 + m)); // OP_1 + (m-1)
    for (const auto& pk : pubs) {
        script.push_back(static_cast<uint8_t>(pk.size()));
        script.insert(script.end(), pk.begin(), pk.end());
    }
    script.push_back(static_cast<uint8_t>(0x50 + pubs.size()));
    script.push_back(0xae); // OP_CHECKMULTISIG
    return script;
}

Transaction WalletBackend::CreateMultisigSpend(const std::vector<TxOut>& outputs, const std::vector<OutPoint>& coins, const std::vector<PrivKey>& keys, uint8_t threshold, uint64_t fee)
{
    if (keys.size() < threshold) throw std::runtime_error("not enough keys");
    Transaction tx;
    tx.vout = outputs;
    uint64_t inTotal = 0;
    for (const auto& c : coins) {
        TxIn in;
        in.prevout = c;
        in.sequence = 0xffffffff;
        tx.vin.push_back(in);
        auto maybe = m_lookup ? m_lookup(c) : std::optional<TxOut>{};
        if (!maybe) throw std::runtime_error("missing utxo");
        inTotal += maybe->value;
    }
    if (inTotal < fee) throw std::runtime_error("fee too high");
    if (inTotal > fee) {
        TxOut change{inTotal - fee, {0x6a}};
        tx.vout.push_back(change);
    }

    for (size_t i = 0; i < tx.vin.size(); ++i) {
        std::vector<uint8_t> sigBlob;
        sigBlob.push_back(0x00); // multisig bug compat
        for (size_t k = 0; k < threshold; ++k) {
            auto sig = SignDigest(keys[k], tx, i);
            sigBlob.push_back(static_cast<uint8_t>(sig.size()));
            sigBlob.insert(sigBlob.end(), sig.begin(), sig.end());
        }
        tx.vin[i].scriptSig = sigBlob;
    }
    RemoveCoins(coins);
    return tx;
}

void WalletBackend::RemoveCoins(const std::vector<OutPoint>& used)
{
    std::lock_guard<std::mutex> g(m_mutex);
    std::vector<UTXO> remaining;
    remaining.reserve(m_utxos.size());
    for (const auto& u : m_utxos) {
        bool spent = false;
        for (const auto& op : used) {
            if (u.outpoint.hash == op.hash && u.outpoint.index == op.index) { spent = true; break; }
        }
        if (!spent) remaining.push_back(u);
    }
    m_utxos.swap(remaining);
}

} // namespace wallet

