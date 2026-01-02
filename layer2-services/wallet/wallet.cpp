#include "wallet.h"

#include "../../layer1-core/crypto/schnorr.h"

#include <algorithm>
#include <memory>
#include <openssl/ec.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>
#include <optional>
#include <stdexcept>

namespace wallet {
namespace {

using ec_group_ptr = std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)>;
using ec_point_ptr = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>;
using bn_ptr = std::unique_ptr<BIGNUM, decltype(&BN_clear_free)>;
using bn_ctx_ptr = std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)>;
constexpr size_t XONLY_PUBKEY_SIZE = 32;

ec_group_ptr make_group()
{
    return ec_group_ptr(EC_GROUP_new_by_curve_name(NID_secp256k1), &EC_GROUP_free);
}

bool valid_secret(const BIGNUM* k, const BIGNUM* order)
{
    return k && order && BN_is_zero(k) == 0 && BN_is_negative(k) == 0 && BN_cmp(k, order) < 0;
}

bn_ptr bn_from_bytes(const uint8_t* data, size_t len)
{
    return bn_ptr(BN_bin2bn(data, static_cast<int>(len), nullptr), &BN_clear_free);
}

std::vector<uint8_t> to_xonly(const PubKey& pub)
{
    if (pub.size() != XONLY_PUBKEY_SIZE + 1) throw std::runtime_error("unexpected pubkey size");
    return std::vector<uint8_t>(pub.begin() + 1, pub.begin() + 1 + XONLY_PUBKEY_SIZE);
}

bool bn_to_32(const BIGNUM* bn, uint8_t out[32])
{
    return BN_bn2binpad(bn, out, 32) == 32;
}

PubKey derive_pubkey(const PrivKey& priv)
{
    ec_group_ptr group = make_group();
    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    if (!group || !ctx) {
        throw std::runtime_error("failed to allocate EC context");
    }
    bn_ptr scalar = bn_from_bytes(priv.data(), priv.size());
    if (!scalar) {
        throw std::runtime_error("invalid private key");
    }
    ec_point_ptr point(EC_POINT_new(group.get()), &EC_POINT_free);
    if (!point) {
        throw std::runtime_error("failed to allocate point");
    }
    if (EC_POINT_mul(group.get(), point.get(), scalar.get(), nullptr, nullptr, ctx.get()) != 1) {
        throw std::runtime_error("EC_POINT_mul failed");
    }
    PubKey out{};
    size_t written = EC_POINT_point2oct(group.get(), point.get(), POINT_CONVERSION_COMPRESSED, out.data(), out.size(), ctx.get());
    if (written != out.size()) {
        throw std::runtime_error("pubkey serialization failed");
    }
    return out;
}

uint32_t fingerprint(const PubKey& pub)
{
    uint8_t sha_out[SHA256_DIGEST_LENGTH]{};
    SHA256(pub.data(), pub.size(), sha_out);
    uint8_t ripe_out[RIPEMD160_DIGEST_LENGTH]{};
    RIPEMD160(sha_out, SHA256_DIGEST_LENGTH, ripe_out);
    uint32_t fp = 0;
    for (int i = 0; i < 4; ++i) {
        fp = (fp << 8) | ripe_out[i];
    }
    return fp;
}

void enforce_single_asset(std::optional<uint8_t>& current, uint8_t candidate)
{
    if (current && *current != candidate)
        throw std::runtime_error("cannot mix asset types in a single transaction");
    current = candidate;
}

KeyId make_key_id(const PrivKey& priv)
{
    KeyId id{};
    SHA256(priv.data(), priv.size(), id.data());
    return id;
}

} // namespace

WalletBackend::WalletBackend(KeyStore store)
    : m_store(std::move(store)) {}

KeyId WalletBackend::ImportKey(const PrivKey& priv)
{
    KeyId id = make_key_id(priv);
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

uint64_t WalletBackend::GetBalance(uint8_t assetId) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    uint64_t total = 0;
    for (const auto& u : m_utxos) {
        if (u.txout.assetId == assetId)
            total += u.txout.value;
    }
    return total;
}

std::unordered_map<uint8_t, uint64_t> WalletBackend::GetBalances() const
{
    std::lock_guard<std::mutex> g(m_mutex);
    std::unordered_map<uint8_t, uint64_t> totals;
    for (const auto& u : m_utxos) totals[u.txout.assetId] += u.txout.value;
    return totals;
}

std::vector<UTXO> WalletBackend::SelectCoins(uint64_t amount, std::optional<uint8_t> assetId) const
{
    std::vector<UTXO> chosen;
    uint64_t acc = 0;
    for (const auto& u : m_utxos) {
        if (assetId && u.txout.assetId != *assetId) continue;
        chosen.push_back(u);
        acc += u.txout.value;
        if (acc >= amount) break;
    }
    if (acc < amount) throw std::runtime_error("insufficient funds");
    return chosen;
}

PubKey WalletBackend::DerivePub(const PrivKey& priv) const
{
    return derive_pubkey(priv);
}

std::vector<uint8_t> WalletBackend::SignDigest(const PrivKey& key, const Transaction& tx, size_t inputIndex) const
{
    auto digest = ComputeInputDigest(tx, inputIndex);
    std::array<uint8_t, 64> sig{};
    std::array<uint8_t, 32> aux{};
    unsigned int aux_len = 0;
    HMAC(EVP_sha256(), key.data(), key.size(), digest.data(), digest.size(), aux.data(), &aux_len);
    if (aux_len != 32u)
        throw std::runtime_error("deterministic aux must be 32 bytes");
    if (!schnorr_sign_with_aux(key.data(), digest.data(), aux.data(), sig.data()))
        throw std::runtime_error("schnorr sign failed");
    return std::vector<uint8_t>(sig.begin(), sig.end());
}

Transaction WalletBackend::CreateSpend(const std::vector<TxOut>& outputs, const KeyId& from, uint64_t fee)
{
    PrivKey key;
    if (!m_store.Get(from, key)) throw std::runtime_error("missing key");
    uint64_t value = fee;
    for (const auto& o : outputs) value += o.value;
    std::optional<uint8_t> spendAsset;
    for (const auto& o : outputs) enforce_single_asset(spendAsset, o.assetId);
    auto coins = SelectCoins(value, spendAsset);
    if (coins.empty())
        throw std::runtime_error("no inputs selected");

    Transaction tx;
    tx.vout = outputs;
    uint64_t inTotal = 0;
    for (const auto& c : coins) {
        enforce_single_asset(spendAsset, c.txout.assetId);
        TxIn in;
        in.prevout = c.outpoint;
        in.sequence = 0xffffffff;
        in.assetId = c.txout.assetId;
        tx.vin.push_back(in);
        inTotal += c.txout.value;
    }
    if (!spendAsset)
        throw std::runtime_error("missing asset for spend");
    if (inTotal > value) {
        auto changeScript = to_xonly(derive_pubkey(key));
        if (changeScript.size() != XONLY_PUBKEY_SIZE) throw std::runtime_error("invalid pubkey size for change output");
        TxOut change{inTotal - value, changeScript};
        change.assetId = *spendAsset;
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

void WalletBackend::SetHDSeed(const std::vector<uint8_t>& seed)
{
    if (seed.empty()) throw std::runtime_error("seed must not be empty");
    unsigned int len = 0;
    std::array<uint8_t, 64> I{};
    HMAC(EVP_sha512(), "Bitcoin seed", 12, seed.data(), seed.size(), I.data(), &len);

    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    ec_group_ptr group = make_group();
    bn_ptr order(BN_new(), &BN_clear_free);
    if (!ctx || !group || !order || EC_GROUP_get_order(group.get(), order.get(), ctx.get()) != 1) {
        throw std::runtime_error("failed to load secp256k1 order");
    }

    bn_ptr il = bn_from_bytes(I.data(), 32);
    if (!il || !valid_secret(il.get(), order.get())) {
        throw std::runtime_error("invalid master key material");
    }

    std::copy(I.begin(), I.begin() + 32, m_master.priv.begin());
    std::copy(I.begin() + 32, I.begin() + 64, m_master.chainCode.begin());
    m_master.pub = derive_pubkey(m_master.priv);
    m_master.depth = 0;
    m_master.childNumber = 0;
    m_master.parentFingerprint = 0;
    m_hasSeed = true;

    ImportKey(m_master.priv);
}

HDNode WalletBackend::DeriveChild(const HDNode& node, uint32_t index, bool hardened)
{
    if (!m_hasSeed) throw std::runtime_error("missing seed");
    uint32_t child_index = hardened ? (index | 0x80000000u) : index;

    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    ec_group_ptr group = make_group();
    bn_ptr order(BN_new(), &BN_clear_free);
    if (!ctx || !group || !order || EC_GROUP_get_order(group.get(), order.get(), ctx.get()) != 1) {
        throw std::runtime_error("failed to load secp256k1 order");
    }

    HDNode child{};
    child.depth = node.depth + 1;
    child.childNumber = child_index;
    child.parentFingerprint = fingerprint(node.pub);

    std::array<uint8_t, 37> data{};
    if (hardened) {
        data[0] = 0x00;
        std::copy(node.priv.begin(), node.priv.end(), data.begin() + 1);
    } else {
        std::copy(node.pub.begin(), node.pub.end(), data.begin());
    }
    data[33] = static_cast<uint8_t>(child_index >> 24);
    data[34] = static_cast<uint8_t>(child_index >> 16);
    data[35] = static_cast<uint8_t>(child_index >> 8);
    data[36] = static_cast<uint8_t>(child_index);

    std::array<uint8_t, 64> I{};
    unsigned int len = 0;
    HMAC(EVP_sha512(), node.chainCode.data(), node.chainCode.size(), data.data(), data.size(), I.data(), &len);

    bn_ptr il = bn_from_bytes(I.data(), 32);
    bn_ptr parent_k = bn_from_bytes(node.priv.data(), node.priv.size());
    if (!il || !parent_k || BN_mod_add(il.get(), il.get(), parent_k.get(), order.get(), ctx.get()) != 1) {
        throw std::runtime_error("failed deriving child scalar");
    }
    if (!valid_secret(il.get(), order.get())) {
        return DeriveChild(node, index + 1, hardened); // skip invalid child per BIP-32
    }
    if (!bn_to_32(il.get(), child.priv.data())) {
        throw std::runtime_error("failed serializing child priv");
    }
    std::copy(I.begin() + 32, I.begin() + 64, child.chainCode.begin());
    child.pub = derive_pubkey(child.priv);

    ImportKey(child.priv);
    return child;
}

HDNode WalletBackend::DeriveBip44(uint32_t account, uint32_t change, uint32_t address_index)
{
    const uint32_t purpose = 44u | 0x80000000u;
    const uint32_t coin_type = 0u | 0x80000000u; // DRACHMA reuses Bitcoin-like mainnet coin type 0 for now
    HDNode account_node = DeriveChild(m_master, purpose, true);
    account_node = DeriveChild(account_node, coin_type, true);
    account_node = DeriveChild(account_node, account | 0x80000000u, true);
    HDNode change_node = DeriveChild(account_node, change, false);
    return DeriveChild(change_node, address_index, false);
}

PubKey WalletBackend::GenerateAddress(uint32_t account, uint32_t change, uint32_t address_index)
{
    HDNode leaf = DeriveBip44(account, change, address_index);
    return leaf.pub;
}

bool WalletBackend::SchnorrSign(const HDNode& node, const std::array<uint8_t, 32>& msg_hash, std::array<uint8_t, 64>& sig_out) const
{
    return schnorr_sign(node.priv.data(), msg_hash.data(), sig_out.data());
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
    std::optional<uint8_t> spendAsset;
    for (const auto& o : outputs) enforce_single_asset(spendAsset, o.assetId);
    uint64_t inTotal = 0;
    std::optional<TxOut> changeTemplate;
    for (const auto& prev : coins) {
        TxIn in;
        in.prevout = prev;
        in.sequence = 0xffffffff;
        tx.vin.push_back(in);
        auto maybe = m_lookup ? m_lookup(prev) : std::optional<TxOut>{};
        if (!maybe) throw std::runtime_error("missing utxo");
        enforce_single_asset(spendAsset, maybe->assetId);
        tx.vin.back().assetId = maybe->assetId;
        if (!changeTemplate) {
            changeTemplate = *maybe;
        } else if (changeTemplate->scriptPubKey != maybe->scriptPubKey) {
            // Keep change under the same locking script as the gathered inputs to avoid weakening spend conditions when mixing policies.
            throw std::runtime_error("cannot create change output: input UTXOs have different script types");
        }
        inTotal += maybe->value;
    }
    if (inTotal < fee) throw std::runtime_error("fee too high");
    if (inTotal > fee) {
        if (!changeTemplate) throw std::runtime_error("missing change template");
        // Multisig change must mirror the gathered script, not the single-sig x-only helper used in CreateSpend.
        if (!spendAsset) throw std::runtime_error("missing asset for change output");
        TxOut change{inTotal - fee, changeTemplate->scriptPubKey};
        change.assetId = *spendAsset;
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
