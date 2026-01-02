#include "validation.h"
#include "../pow/difficulty.h"
#include "../merkle/merkle.h"
#include "../script/interpreter.h"
#include <openssl/crypto.h>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <optional>

namespace {

constexpr uint8_t kPowAssetId = static_cast<uint8_t>(AssetId::TALANTON);

bool IsNullOutPoint(const OutPoint& prevout)
{
    return std::all_of(prevout.hash.begin(), prevout.hash.end(), [](uint8_t b) { return b == 0; }) &&
           prevout.index == std::numeric_limits<uint32_t>::max();
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

bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params, int height, const UTXOLookup& lookup, bool posMode, uint32_t posBits, uint32_t posTime)
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

    if (!posMode) {
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

        for (size_t i = 0; i < txs.size(); ++i) {
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

            if (i == 0) {
                if (!checkAsset(txAsset, tx.vin.front().assetId))
                    return false;
                continue;
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

    // PoS path
    if (IsCoinbase(txs.front()))
        return false;
    if (!lookup)
        return false;

    const Transaction& stakeTx = txs.front();
    std::optional<uint8_t> stakeAsset;
    if (stakeTx.vin.size() != 1 || stakeTx.vout.size() < 2)
        return false;
    if (stakeTx.vout.front().value != 0)
        return false;
    for (const auto& out : stakeTx.vout) {
        if (!checkAsset(stakeAsset, out.assetId))
            return false;
    }

    const TxIn& stakeIn = stakeTx.vin.front();
    if (IsNullOutPoint(stakeIn.prevout))
        return false;
    if (stakeIn.scriptSig.empty() || stakeIn.scriptSig.size() > 1650)
        return false;
    if (!checkAsset(stakeAsset, stakeIn.assetId))
        return false;

    if (multiAssetActive) {
        const auto& policy = consensus::GetAssetPolicy(*stakeAsset);
        if (!policy.posAllowed)
            return false;
    }

    auto stakedUtxo = cachedLookup(stakeIn.prevout);
    if (!stakedUtxo || stakeIn.assetId != stakedUtxo->assetId || !checkAsset(stakeAsset, stakedUtxo->assetId))
        return false;
    if (stakedUtxo->scriptPubKey.size() != 32)
        return false;

    auto append32 = [](std::vector<uint8_t>& dst, const uint256& h) {
        dst.insert(dst.end(), h.begin(), h.end());
    };
    std::vector<uint8_t> kernel;
    kernel.reserve(32 + 4 + 4 + stakedUtxo->scriptPubKey.size());
    append32(kernel, stakeIn.prevout.hash);
    kernel.push_back(static_cast<uint8_t>(stakeIn.prevout.index & 0xff));
    kernel.push_back(static_cast<uint8_t>((stakeIn.prevout.index >> 8) & 0xff));
    kernel.push_back(static_cast<uint8_t>((stakeIn.prevout.index >> 16) & 0xff));
    kernel.push_back(static_cast<uint8_t>((stakeIn.prevout.index >> 24) & 0xff));
    kernel.push_back(static_cast<uint8_t>(posTime & 0xff));
    kernel.push_back(static_cast<uint8_t>((posTime >> 8) & 0xff));
    kernel.push_back(static_cast<uint8_t>((posTime >> 16) & 0xff));
    kernel.push_back(static_cast<uint8_t>((posTime >> 24) & 0xff));
    kernel.insert(kernel.end(), stakedUtxo->scriptPubKey.begin(), stakedUtxo->scriptPubKey.end());

    uint256 kernelHash = tagged_hash("STAKE", kernel.data(), kernel.size());
    if (!powalgo::CheckProofOfWork(kernelHash, posBits, params))
        return false;

    uint64_t totalInputs = 0;
    uint64_t totalOutputs = 0;

    auto addSafe = [&](uint64_t a, uint64_t b, uint64_t& out) {
        if (!SafeAdd(a, b, out))
            return false;
        return true;
    };

    uint64_t nextVal = 0;
    if (!addSafe(totalInputs, stakedUtxo->value, nextVal))
        return false;
    totalInputs = nextVal;
    if (!consensus::MoneyRange(totalInputs, params))
        return false;

    for (const auto& out : stakeTx.vout) {
        uint64_t nxt = 0;
        if (!addSafe(totalOutputs, out.value, nxt))
            return false;
        totalOutputs = nxt;
        const uint8_t rangeAsset = stakeAsset.value_or(out.assetId);
        if (!consensus::MoneyRange(out.value, params, rangeAsset) || !consensus::MoneyRange(totalOutputs, params, rangeAsset))
            return false;
        if (out.scriptPubKey.size() != 32)
            return false;
        if (&out != &stakeTx.vout.front() && out.value < DUST_THRESHOLD)
            return false;
    }

    for (size_t i = 1; i < txs.size(); ++i) {
        const auto& tx = txs[i];
        std::optional<uint8_t> txAsset;
        if (IsCoinbase(tx))
            return false;

        const size_t txSize = Serialize(tx).size();
        if (txSize == 0 || txSize > MAX_TX_SIZE)
            return false;
        runningWeight += txSize * 4;
        if (runningWeight > MAX_BLOCK_WEIGHT)
            return false;

        if (tx.vin.empty() || tx.vout.empty())
            return false;

        uint64_t txInSum = 0;
        uint64_t txOutSum = 0;

        for (const auto& out : tx.vout) {
            if (!checkAsset(txAsset, out.assetId))
                return false;
            uint64_t nxt = 0;
            if (!addSafe(txOutSum, out.value, nxt))
                return false;
            txOutSum = nxt;
            const uint8_t assetForRange = txAsset.value_or(out.assetId);
            if (!consensus::MoneyRange(out.value, params, assetForRange) || !consensus::MoneyRange(txOutSum, params, assetForRange))
                return false;
            if (out.scriptPubKey.size() != 32)
                return false;
            if (out.value < DUST_THRESHOLD)
                return false;
        }

        for (size_t inIdx = 0; inIdx < tx.vin.size(); ++inIdx) {
            const auto& in = tx.vin[inIdx];
            if (IsNullOutPoint(in.prevout))
                return false;
            if (in.scriptSig.empty() || in.scriptSig.size() > 1650)
                return false;
            if (!checkAsset(txAsset, in.assetId))
                return false;
            if (!seenPrevouts.insert(in.prevout).second)
                return false;
            auto utxo = cachedLookup(in.prevout);
            if (!utxo || in.assetId != utxo->assetId || !checkAsset(txAsset, utxo->assetId))
                return false;
            uint64_t nxt = 0;
            if (!addSafe(txInSum, utxo->value, nxt))
                return false;
            txInSum = nxt;
            if (!consensus::MoneyRange(utxo->value, params, txAsset.value_or(in.assetId)) || !consensus::MoneyRange(txInSum, params, txAsset.value_or(in.assetId)))
                return false;
            if (!VerifyScript(tx, inIdx, *utxo))
                return false;
        }

        if (txInSum < txOutSum)
            return false;

        uint64_t nxt = 0;
        if (!addSafe(totalInputs, txInSum, nxt))
            return false;
        totalInputs = nxt;
        if (!addSafe(totalOutputs, txOutSum, nxt))
            return false;
        totalOutputs = nxt;
        const uint8_t assetForRange = txAsset.value_or(stakeAsset.value_or(static_cast<uint8_t>(AssetId::DRACHMA)));
        if (!consensus::MoneyRange(totalInputs, params, assetForRange) || !consensus::MoneyRange(totalOutputs, params, assetForRange))
            return false;
    }

    uint64_t subsidy = multiAssetActive && stakeAsset
        ? consensus::GetPoSReward(stakedUtxo->value, params, *stakeAsset)
        : consensus::GetBlockSubsidy(height, params) * params.nPoSRewardRatioNum / params.nPoSRewardRatioDen;
    if (totalOutputs < totalInputs)
        return false;
    if (totalOutputs - totalInputs > subsidy)
        return false;

    return true;
}

bool ValidateBlock(const Block& block, const consensus::Params& params, int height, const UTXOLookup& lookup, const BlockValidationOptions& opts)
{
    const bool posAllowed = params.fHybridPoS && height >= static_cast<int>(params.nPoSActivationHeight);
    const bool isPoS = posAllowed && !block.transactions.empty() && !IsCoinbase(block.transactions.front());

    if (isPoS) {
        if ((block.header.time % 2) != 0)
            return false;
    }

    if (!ValidateBlockHeader(block.header, params, opts, isPoS))
        return false;
    if (!ValidateTransactions(block.transactions, params, height, lookup, isPoS, block.header.bits, block.header.time))
        return false;
    const auto merkle = ComputeMerkleRoot(block.transactions);
    if (CRYPTO_memcmp(merkle.data(), block.header.merkleRoot.data(), merkle.size()) != 0)
        return false;
    return true;
}
