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
#include <unordered_set>

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
    uint8_t sha_out[32]{};
    unsigned int sha_len = 32;
    // Reuse single context for both hash operations
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    
    // Compute SHA256
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, pub.data(), pub.size()) != 1 ||
        EVP_DigestFinal_ex(ctx, sha_out, &sha_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA256 failed");
    }
    
    // Compute RIPEMD160 using the same context
    uint8_t ripe_out[20]{};
    unsigned int ripe_len = 20;
    if (EVP_DigestInit_ex(ctx, EVP_ripemd160(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, sha_out, 32) != 1 ||
        EVP_DigestFinal_ex(ctx, ripe_out, &ripe_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("RIPEMD160 failed");
    }
    EVP_MD_CTX_free(ctx);
    
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
    unsigned int len = 32;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, priv.data(), priv.size()) != 1 ||
        EVP_DigestFinal_ex(ctx, id.data(), &len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA256 failed");
    }
    EVP_MD_CTX_free(ctx);
    return id;
}

struct OutPointHasher {
    std::size_t operator()(const OutPoint& o) const noexcept {
        size_t h = 0;
        for (auto b : o.hash) h = (h * 131) ^ b;
        h ^= static_cast<size_t>(o.index + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
        return h;
    }
};

struct OutPointEqual {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept {
        return a.index == b.index && std::equal(a.hash.begin(), a.hash.end(), b.hash.begin());
    }
};

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
    
    // Build hash set of existing UTXOs for O(1) lookups
    std::unordered_set<OutPoint, OutPointHasher, OutPointEqual> existingSet;
    for (const auto& u : m_utxos) {
        existingSet.insert(u.outpoint);
    }
    
    for (const auto& op : watchlist) {
        if (existingSet.count(op)) continue;
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
    // Enhanced coin selection with multiple strategies for optimal UTXO management:
    // 1. Exact match: Find single UTXO that exactly matches target amount
    // 2. Single larger: Use single UTXO larger than target (minimizes inputs)
    // 3. Accumulative: Smallest-first accumulation (reduces fragmentation)
    // 4. Fallback: Simple first-fit if other strategies fail
    
    std::vector<UTXO> candidates;
    candidates.reserve(m_utxos.size());
    for (const auto& u : m_utxos) {
        if (assetId && u.txout.assetId != *assetId) continue;
        candidates.push_back(u);
    }
    
    if (candidates.empty()) throw std::runtime_error("no UTXOs available");
    
    // Strategy 1: Look for exact match (ideal case - no change output needed)
    for (const auto& u : candidates) {
        if (u.txout.value == amount) {
            return std::vector<UTXO>{u};
        }
    }
    
    // Strategy 2: Find smallest single UTXO that covers amount (minimizes inputs)
    std::optional<UTXO> bestSingle;
    for (const auto& u : candidates) {
        if (u.txout.value >= amount) {
            if (!bestSingle || u.txout.value < bestSingle->txout.value) {
                bestSingle = u;
            }
        }
    }
    
    if (bestSingle) {
        return std::vector<UTXO>{*bestSingle};
    }
    
    // Strategy 3: Accumulative selection - use smallest UTXOs first to reduce fragmentation
    // This helps consolidate dust and maintains larger UTXOs for future larger transactions
    // Simple single pass: sort and accumulate until target is reached
    std::vector<UTXO> sortedCandidates = candidates;
    std::sort(sortedCandidates.begin(), sortedCandidates.end(),
              [](const UTXO& a, const UTXO& b) { return a.txout.value < b.txout.value; });
    
    std::vector<UTXO> chosen;
    uint64_t acc = 0;
    for (const auto& u : sortedCandidates) {
        chosen.push_back(u);
        acc += u.txout.value;
        if (acc >= amount) {
            return chosen;
        }
    }
    
    // If we reach here, insufficient funds
    throw std::runtime_error("insufficient funds");
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
    if (used.empty()) return;
    
    // Use hash set for O(1) lookups instead of O(n) nested loops
    std::unordered_set<OutPoint, OutPointHasher, OutPointEqual> spentSet;
    for (const auto& op : used) {
        spentSet.insert(op);
    }
    
    std::vector<UTXO> remaining;
    remaining.reserve(m_utxos.size());
    for (const auto& u : m_utxos) {
        if (spentSet.find(u.outpoint) == spentSet.end()) {
            remaining.push_back(u);
        }
    }
    m_utxos.swap(remaining);
}

} // namespace wallet
