#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <limits>
#include <string>
#include <vector>
#include "../block/block.h"

namespace consensus {

struct VBDeployment {
    // Bit position to signal in the block version field.
    int bit;

    // Start and timeout are expressed as UNIX timestamps (seconds).
    // A value of -1 for start disables the deployment, while -1 for timeout
    // keeps it active indefinitely once started.
    int64_t nStartTime;
    int64_t nTimeout;
};

enum DeploymentPos {
    DEPLOYMENT_TESTDUMMY = 0,
    MAX_VERSION_BITS_DEPLOYMENTS
};

struct Params {
    uint32_t nSubsidyHalvingInterval;
    uint32_t nPowTargetSpacing;
    uint32_t nPowTargetTimespan;
    uint32_t nDifficultyAdjustmentInterval;
    uint64_t nMaxMoneyOut;

    bool fPowAllowMinDifficultyBlocks;

    uint32_t nGenesisTime;
    uint32_t nGenesisBits;
    uint32_t nGenesisNonce;
    std::string genesisMessage;

    // Optional hardened checkpoints keyed by height. Chains should not accept
    // competing headers that disagree with these anchors.
    std::map<uint32_t, uint256> checkpoints;

    // Version bits governance-free activation parameters.
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    std::array<VBDeployment, MAX_VERSION_BITS_DEPLOYMENTS> vDeployments;

    // Hybrid PoW/PoS controls
    bool fHybridPoS{false};
    uint32_t nPoSActivationHeight{std::numeric_limits<uint32_t>::max()};
    uint32_t nPoSMinStakeDepth{0};
    uint32_t nPoSTargetSpacing{0};
    uint32_t nPoSRewardRatioNum{1};
    uint32_t nPoSRewardRatioDen{2};

    // Multi-asset activation height (regenesis/fork point).
    uint32_t nMultiAssetActivationHeight{0};
};

const Params& Main();
const Params& Testnet();

// Asset-specific monetary policy.
struct AssetPolicy {
    uint8_t assetId;
    bool powAllowed;
    bool posAllowed;
    uint32_t powHalvingInterval;
    uint64_t powInitialSubsidy;
    uint64_t maxMoney;
    uint32_t posSlotSpacing;
    double posApr;
    bool posEth2Curve;
    uint64_t posSupplyTarget;
    uint32_t minStakeAgeSlots;
};

const AssetPolicy& GetAssetPolicy(uint8_t assetId);
std::vector<AssetPolicy> GetAllAssetPolicies();
const char* AssetSymbol(uint8_t assetId);
bool ParseAssetSymbol(const std::string& symbol, uint8_t& out);
bool IsMultiAssetActive(const Params& params, int height);

// Monetary policy helpers.
uint64_t GetBlockSubsidy(int height, const Params& params);
uint64_t GetBlockSubsidy(int height, const Params& params, uint8_t assetId);
uint64_t GetPoSReward(uint64_t stakeValue, const Params& params, uint8_t assetId);
uint64_t GetMaxMoney(const Params& params, uint8_t assetId);
uint64_t GetMaxMoney(const Params& params);
bool MoneyRange(uint64_t amount, const Params& params, uint8_t assetId);
bool MoneyRange(uint64_t amount, const Params& params);

} // namespace consensus
