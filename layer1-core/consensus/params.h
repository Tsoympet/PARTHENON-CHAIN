#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <string>
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
};

const Params& Main();
const Params& Testnet();

// Monetary policy helpers.
uint64_t GetBlockSubsidy(int height, const Params& params);
uint64_t GetMaxMoney(const Params& params);
bool MoneyRange(uint64_t amount, const Params& params);

} // namespace consensus
