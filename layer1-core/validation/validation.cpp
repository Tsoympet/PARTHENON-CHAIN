#include "validation.h"
#include "../pow/difficulty.h"
#include "../merkle/merkle.h"
#include "../script/interpreter.h"
#include <openssl/crypto.h>
#include <array>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <optional>

namespace {

constexpr uint8_t kPowAssetId = static_cast<uint8_t>(AssetId::TALANTON);
const std::array<uint8_t, 32> kEmptyRoot{};

bool IsNullOutPoint(const OutPoint& prevout)
{
    // Check index first (cheaper than iterating hash)
    if (prevout.index != std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    // Manual loop with early exit is faster than std::all_of
    for (auto b : prevout.hash) {
        if (b != 0) return false;
    }
    return true;
}

struct OutPointHasher {
    std::size_t operator()(const OutPoint& o) const noexcept
    {
        size_t h = 0;
        for (auto b : o.hash) h = (h * 131) ^ b;
        h ^= static_cast<size_t>(o.index + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
        return h;
    }
};

struct OutPointEq {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept
    {
        return a.index == b.index && std::equal(a.hash.begin(), a.hash.end(), b.hash.begin());
    }
};

bool SafeAdd(uint64_t a, uint64_t b, uint64_t& out)
{
    if (a > std::numeric_limits<uint64_t>::max() - b)
        return false;
    out = a + b;
    return true;
}

bool IsCoinbase(const Transaction& tx)
{
    return tx.vin.size() == 1 && IsNullOutPoint(tx.vin.front().prevout);
}

} // namespace

bool ValidateBlockHeader(const BlockHeader& header, const consensus::Params& params, const BlockValidationOptions& opts, bool skipPowCheck)
{
    if (opts.limiter && !opts.limiter->Consume(opts.limiterWeight))
        return false;

    if (!skipPowCheck) {
        if (!powalgo::CheckProofOfWork(BlockHash(header), header.bits, params))
            return false;
    }

    // Enforce sane timestamp ordering relative to median past and against the
    // wall clock with modest drift tolerance. medianTimePast must be supplied
    // by the caller; using zero would skip the ordering rule and is rejected.
    if (opts.medianTimePast == 0 || header.time <= opts.medianTimePast)
        return false;

    uint64_t horizon = static_cast<uint64_t>(opts.now) + opts.maxFutureDrift;
    uint32_t clampedHorizon = horizon > std::numeric_limits<uint32_t>::max()
        ? std::numeric_limits<uint32_t>::max()
        : static_cast<uint32_t>(horizon);
    if (header.time > clampedHorizon)
        return false;

    return true;
}

namespace {

class CachedLookup {
public:
    CachedLookup(const UTXOLookup& base, size_t capacity)
        : m_base(base), m_capacity(capacity) {}

    std::optional<TxOut> operator()(const OutPoint& out)
    {
        auto it = m_cache.find(out);
        if (it != m_cache.end())
            return it->second;

        auto res = m_base ? m_base(out) : std::nullopt;
        if (res) {
            if (m_cache.size() >= m_capacity) {
                // Simple clock-sweep eviction for predictable behaviour.
                m_cache.erase(m_cache.begin());
            }
            m_cache.emplace(out, *res);
        }
        return res;
    }

private:
    UTXOLookup m_base;
    size_t m_capacity;
    std::unordered_map<OutPoint, TxOut, OutPointHasher, OutPointEq> m_cache;
};

} // namespace

bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params, int height, const UTXOLookup& lookup)
{
    if (txs.empty()) return false;

    const bool multiAssetActive = consensus::IsMultiAssetActive(params, height);
    constexpr size_t MAX_TX_SIZE = 1000000; // 1MB hard cap per tx
    constexpr size_t MAX_BLOCK_WEIGHT = 4000000; // approximate weight limit
    constexpr uint64_t DUST_THRESHOLD = 546; // satoshi-equivalent dust floor

    std::unordered_set<OutPoint, OutPointHasher, OutPointEq> seenPrevouts;
    seenPrevouts.reserve(txs.size() * 2);
    size_t runningWeight = 0;
    CachedLookup cachedLookup(lookup, 1024);

    auto checkAsset = [](std::optional<uint8_t>& asset, uint8_t candidate) {
        if (!IsValidAssetId(candidate))
            return false;
        if (asset && *asset != candidate)
            return false;
        asset = candidate;
        return true;
    };

    // Coinbase must be first and unique
    if (!IsCoinbase(txs.front()))
        return false;

    if (txs.front().vout.empty())
        return false;

    // Enforce reasonable scriptSig length on coinbase (2-100 bytes)
    const auto& coinbaseSig = txs.front().vin.front().scriptSig;
    if (coinbaseSig.size() < 2 || coinbaseSig.size() > 100)
        return false;

    uint64_t coinbaseOutTotal = 0;
    std::optional<uint8_t> coinbaseAsset;
    for (const auto& out : txs.front().vout) {
        if (!checkAsset(coinbaseAsset, out.assetId))
            return false;
        uint64_t next = 0;
        if (!SafeAdd(coinbaseOutTotal, out.value, next))
            return false;
        coinbaseOutTotal = next;
        const uint8_t assetForRange = coinbaseAsset.value_or(static_cast<uint8_t>(AssetId::DRACHMA));
        if (!consensus::MoneyRange(out.value, params, assetForRange) || !consensus::MoneyRange(coinbaseOutTotal, params, assetForRange))
            return false;
        if (out.scriptPubKey.size() != 32)
            return false; // enforce schnorr-only pubkeys
    }
    if (!coinbaseAsset || !checkAsset(coinbaseAsset, txs.front().vin.front().assetId))
        return false;

    if (multiAssetActive) {
        if (*coinbaseAsset != kPowAssetId)
            return false;
        const auto& policy = consensus::GetAssetPolicy(*coinbaseAsset);
        if (!policy.powAllowed)
            return false;
    }

    uint64_t totalFees = 0;

    for (size_t i = 1; i < txs.size(); ++i) {
        const auto& tx = txs[i];
        std::optional<uint8_t> txAsset;

        const size_t txSize = Serialize(tx).size();
        if (txSize == 0 || txSize > MAX_TX_SIZE)
            return false;
        runningWeight += txSize * 4; // legacy weight approximation
        if (runningWeight > MAX_BLOCK_WEIGHT)
            return false;

        uint64_t totalOut = 0;
        for (const auto& out : tx.vout) {
            if (!checkAsset(txAsset, out.assetId))
                return false;
            uint64_t next = 0;
            if (!SafeAdd(totalOut, out.value, next))
                return false;
            totalOut = next;
            const uint8_t assetForRange = txAsset.value_or(out.assetId);
            if (!consensus::MoneyRange(out.value, params, assetForRange) || !consensus::MoneyRange(totalOut, params, assetForRange))
                return false;
            if (out.scriptPubKey.size() != 32)
                return false; // enforce schnorr-only pubkeys
            if (out.value < DUST_THRESHOLD)
                return false;
        }

        if (IsCoinbase(tx))
            return false; // only the first tx may be coinbase

        if (!lookup)
            return false; // cannot validate spends without a UTXO provider

        if (tx.vin.empty() || tx.vout.empty())
            return false;

            uint64_t totalIn = 0;
            for (size_t inIdx = 0; inIdx < tx.vin.size(); ++inIdx) {
                const auto& in = tx.vin[inIdx];
                if (IsNullOutPoint(in.prevout))
                    return false;
                if (in.scriptSig.empty())
                    return false;
                if (in.scriptSig.size() > 1650)
                    return false; // oversized scripts risk DoS

                if (!checkAsset(txAsset, in.assetId))
                    return false;
                if (!seenPrevouts.insert(in.prevout).second)
                    return false; // duplicate spend within block

                auto utxo = cachedLookup(in.prevout);
                if (!utxo || in.assetId != utxo->assetId || !checkAsset(txAsset, utxo->assetId))
                    return false;

                if (!VerifyScript(tx, inIdx, *utxo))
                    return false;

                uint64_t next = 0;
                if (!SafeAdd(totalIn, utxo->value, next))
                    return false;
                totalIn = next;
                if (!consensus::MoneyRange(totalIn, params, txAsset.value_or(in.assetId)))
                    return false;
            }

            if (totalOut > totalIn)
                return false; // overspends

            uint64_t fee = totalIn - totalOut;
            uint64_t nextFees = 0;
            if (!SafeAdd(totalFees, fee, nextFees))
                return false;
            totalFees = nextFees;
            if (!consensus::MoneyRange(totalFees, params))
                return false;
        }

    uint64_t maxCoinbase = multiAssetActive && coinbaseAsset
        ? consensus::GetBlockSubsidy(height, params, *coinbaseAsset)
        : consensus::GetBlockSubsidy(height, params);
    if (!SafeAdd(maxCoinbase, totalFees, maxCoinbase))
        return false;

    if (coinbaseOutTotal > maxCoinbase)
        return false;

    return true;
}

bool ValidateBlock(const Block& block, const consensus::Params& params, int height, const UTXOLookup& lookup, const BlockValidationOptions& opts)
{
    if (!ValidateBlockHeader(block.header, params, opts, false))
        return false;
    if (opts.requireNftStateRoot) {
        // Keep validation side-effect free; callers gate acceptance on the boolean result.
        if (opts.nftStateRoot == kEmptyRoot)
            return false;
        if (opts.expectedNftStateRoot != kEmptyRoot &&
            opts.nftStateRoot != opts.expectedNftStateRoot)
            return false;
    }
    if (!ValidateTransactions(block.transactions, params, height, lookup))
        return false;
    const auto merkle = ComputeMerkleRoot(block.transactions);
    if (CRYPTO_memcmp(merkle.data(), block.header.merkleRoot.data(), merkle.size()) != 0)
        return false;
    return true;
}
