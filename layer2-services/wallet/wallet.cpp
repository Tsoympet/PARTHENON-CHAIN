#include "wallet.h"

#include <algorithm>
#include <openssl/sha.h>
#include <stdexcept>

namespace wallet {

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

std::vector<uint8_t> WalletBackend::DummySignature(const PrivKey& key, const Transaction& tx) const
{
    auto ser = Serialize(tx);
    std::vector<uint8_t> out(32);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, key.data(), key.size());
    SHA256_Update(&ctx, ser.data(), ser.size());
    SHA256_Final(out.data(), &ctx);
    return out;
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

    for (auto& in : tx.vin) {
        in.scriptSig = DummySignature(key, tx);
    }
    std::vector<OutPoint> spent;
    spent.reserve(tx.vin.size());
    for (const auto& in : tx.vin) spent.push_back(in.prevout);
    RemoveCoins(spent);
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
