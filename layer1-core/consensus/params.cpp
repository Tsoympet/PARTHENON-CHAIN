#include "params.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include "../tx/transaction.h"

namespace {
constexpr uint64_t COIN = 100000000ULL;
constexpr uint32_t DEFAULT_WINDOW = 2016;                 // ~33.6 hours at 60s blocks
constexpr uint32_t DEFAULT_THRESHOLD = (DEFAULT_WINDOW * 95) / 100; // 95% threshold
}

namespace consensus {

static Params mainParams {
    2102400,           // halving interval (~4 years at 60s blocks)
    60,                // 60 sec block time
    3600,              // 1 hour retarget window
    60,                // 60 blocks per retarget interval
    41000000ULL * COIN,
    false,             // no min-difficulty blocks
    1735689600,        // genesis time
    0x1e0fffff,        // initial difficulty
    0,                 // mined later if not provided
    "DRACHMA: Proof before trust",
    {},                // checkpoints
    DEFAULT_THRESHOLD,
    DEFAULT_WINDOW,
    { VBDeployment{28, -1, -1} },
    false,             // fHybridPoS - disabled, PoW-only
    0,                 // nPoSActivationHeight (disabled)
    0,                 // nPoSMinStakeDepth (disabled)
    0,                 // nPoSTargetSpacing (disabled)
    0,                 // PoS reward numerator (disabled)
    0,                 // PoS reward denominator (disabled)
    1                  // nMultiAssetActivationHeight
};

static Params testParams {
    2102400,
    60,
    3600,
    60,
    41000000ULL * COIN,
    true,              // allow min difficulty
    1735689600,
    0x1f00ffff,
    0,
    "DRACHMA TESTNET",
    {},
    DEFAULT_THRESHOLD,
    DEFAULT_WINDOW,
    { VBDeployment{28, -1, -1} },
    false,             // fHybridPoS - disabled, PoW-only
    0,                 // nPoSActivationHeight (disabled)
    0,                 // nPoSMinStakeDepth (disabled)
    0,                 // nPoSTargetSpacing (disabled)
    0,                 // PoS reward numerator (disabled)
    0,                 // PoS reward denominator (disabled)
    1
};

const Params& Main()    { return mainParams; }
const Params& Testnet() { return testParams; }

namespace {

constexpr uint32_t YEAR_SECONDS = 365 * 24 * 3600;
constexpr double MIN_POS_REWARD_UNIT = 1.0;
constexpr size_t kAssetCount = static_cast<size_t>(AssetId::OBOLOS) + 1;

const AssetPolicy& DefaultPolicy()
{
    static AssetPolicy fallback{
        static_cast<uint8_t>(AssetId::DRACHMA), true, false, 2102400, 10 * COIN, 41000000ULL * COIN, 600, 0.0, false, 41000000ULL * COIN, 0};
    return fallback;
}

} // namespace

const AssetPolicy& GetAssetPolicy(uint8_t assetId)
{
    static const AssetPolicy kPolicies[] = {
        {static_cast<uint8_t>(AssetId::TALANTON), true, false, 2102400, 5 * COIN, 21000000ULL * COIN, 600, 0.0, false, 21000000ULL * COIN, 0},
        {static_cast<uint8_t>(AssetId::DRACHMA), true, false, 2102400, 10 * COIN, 41000000ULL * COIN, 600, 0.0, false, 41000000ULL * COIN, 0},
        {static_cast<uint8_t>(AssetId::OBOLOS), true, false, 2102400, 8 * COIN, 61000000ULL * COIN, 600, 0.0, false, 61000000ULL * COIN, 0},
    };

    for (const auto& policy : kPolicies) {
        if (policy.assetId == assetId)
            return policy;
    }
    return DefaultPolicy();
}

std::vector<AssetPolicy> GetAllAssetPolicies()
{
    std::vector<AssetPolicy> out;
    out.reserve(kAssetCount);
    out.push_back(GetAssetPolicy(static_cast<uint8_t>(AssetId::TALANTON)));
    out.push_back(GetAssetPolicy(static_cast<uint8_t>(AssetId::DRACHMA)));
    out.push_back(GetAssetPolicy(static_cast<uint8_t>(AssetId::OBOLOS)));
    return out;
}

const char* AssetSymbol(uint8_t assetId)
{
    switch (static_cast<AssetId>(assetId)) {
        case AssetId::TALANTON: return "TLN";
        case AssetId::DRACHMA:  return "DRM";
        case AssetId::OBOLOS:   return "OBL";
    }
    return "UNKNOWN";
}

bool ParseAssetSymbol(const std::string& symbol, uint8_t& out)
{
    if (symbol == "TLN" || symbol == "talanton") { out = static_cast<uint8_t>(AssetId::TALANTON); return true; }
    if (symbol == "DRM" || symbol == "drachma")  { out = static_cast<uint8_t>(AssetId::DRACHMA);  return true; }
    if (symbol == "OBL" || symbol == "obolos")   { out = static_cast<uint8_t>(AssetId::OBOLOS);   return true; }
    return false;
}

bool IsMultiAssetActive(const Params& params, int height)
{
    return height >= static_cast<int>(params.nMultiAssetActivationHeight);
}

uint64_t GetBlockSubsidy(int height, const Params& params, uint8_t assetId)
{
    if (height < 0) return 0;

    const auto& policy = GetAssetPolicy(assetId);
    if (!policy.powAllowed)
        return 0;

    const uint32_t halvingInterval = policy.powHalvingInterval ? policy.powHalvingInterval : params.nSubsidyHalvingInterval;
    int halvings = height / static_cast<int>(halvingInterval);
    if (halvings >= 64) // protect against shift overflow
        return 0;

    uint64_t subsidy = policy.powInitialSubsidy ? policy.powInitialSubsidy : 50 * COIN;
    subsidy >>= halvings;
    return subsidy;
}

uint64_t GetBlockSubsidy(int height, const Params& params)
{
    return GetBlockSubsidy(height, params, static_cast<uint8_t>(AssetId::TALANTON));
}

uint64_t GetPoSReward(uint64_t stakeValue, const Params& params, uint8_t assetId)
{
    const auto& policy = GetAssetPolicy(assetId);
    if (!policy.posAllowed || stakeValue == 0)
        return 0;

    const uint32_t slot = policy.posSlotSpacing ? policy.posSlotSpacing : params.nPoSTargetSpacing;
    const double slotsPerYear = slot ? (static_cast<double>(YEAR_SECONDS) / static_cast<double>(slot)) : 0.0;
    if (slotsPerYear == 0.0)
        return 0;

    double annualRate = policy.posApr;
    if (policy.posEth2Curve) {
        // Approximate Eth2-style curve: taper from 5% at low stake toward 1.5% near full participation.
        double participation = static_cast<double>(stakeValue) / static_cast<double>(policy.posSupplyTarget ? policy.posSupplyTarget : policy.maxMoney);
        if (participation > 1.0) participation = 1.0;
        const double maxRate = 0.05;
        const double minRate = 0.015;
        annualRate = maxRate - (maxRate - minRate) * participation;
    }

    if (annualRate <= 0.0)
        return 0;

    double perSlotRate = annualRate / slotsPerYear;
    double reward = static_cast<double>(stakeValue) * perSlotRate;
    if (reward <= 0.0 && stakeValue)
        reward = MIN_POS_REWARD_UNIT; // minimum unit to avoid zeroing tiny stakes
    return static_cast<uint64_t>(reward);
}

uint64_t GetMaxMoney(const Params& params, uint8_t assetId)
{
    const auto& policy = GetAssetPolicy(assetId);
    if (policy.maxMoney)
        return policy.maxMoney;
    return params.nMaxMoneyOut;
}

uint64_t GetMaxMoney(const Params& params)
{
    return GetMaxMoney(params, static_cast<uint8_t>(AssetId::DRACHMA));
}

bool MoneyRange(uint64_t amount, const Params& params)
{
    return MoneyRange(amount, params, static_cast<uint8_t>(AssetId::DRACHMA));
}

bool MoneyRange(uint64_t amount, const Params& params, uint8_t assetId)
{
    return amount <= GetMaxMoney(params, assetId);
}

} // namespace consensus
