#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/consensus/versioning/versionbits.h"
#include "../../layer1-core/block/block.h"
#include <cassert>
#include <stdexcept>

extern Block CreateGenesisBlock(const consensus::Params& params);

namespace {
constexpr uint64_t COIN = 100000000ULL;
}

int main()
{
    const auto& params = consensus::Testnet();

    // TLN is PoW-only; PoS reward must be zero.
    auto tlnReward = consensus::GetPoSReward(100 * COIN, params, static_cast<uint8_t>(AssetId::TALANTON));
    assert(tlnReward == 0);

    // DRM PoS: 4% APR with 10-minute slots.
    const double slotsPerYear = 31536000.0 / 600.0;
    uint64_t stake = 100 * COIN;
    uint64_t expectedDrm = static_cast<uint64_t>((static_cast<double>(stake) * 0.04) / slotsPerYear);
    auto drmReward = consensus::GetPoSReward(stake, params, static_cast<uint8_t>(AssetId::DRACHMA));
    assert(drmReward == expectedDrm);

    // OBL uses Eth2-style curve; for small participation it should exceed DRM's per-slot reward.
    auto oblReward = consensus::GetPoSReward(stake, params, static_cast<uint8_t>(AssetId::OBOLOS));
    assert(oblReward > drmReward);
    assert(oblReward > 0);

    // Version bits guard rails.
    try {
        consensus::VersionBitsMask(consensus::VBDeployment{-1, 0, 0});
        assert(false);
    } catch (const std::invalid_argument&) {}

    consensus::Params vbParams = consensus::Testnet();
    vbParams.nMinerConfirmationWindow = 2;
    vbParams.nRuleChangeActivationThreshold = 1;
    consensus::VBDeployment dep{1, 0, 100};
    std::vector<consensus::BlockVersionSample> history = {
        {0, dep.nStartTime + 1, static_cast<int32_t>(consensus::VersionBitsMask(dep))},
        {1, dep.nStartTime + 2, static_cast<int32_t>(consensus::VersionBitsMask(dep))},
        {2, dep.nStartTime + 3, static_cast<int32_t>(consensus::VersionBitsMask(dep))},
    };
    auto state = consensus::VersionBitsState(vbParams, dep, history);
    assert(state == consensus::ThresholdState::ACTIVE || state == consensus::ThresholdState::LOCKED_IN);

    // Genesis creation rejects mismatched nonce that fails PoW.
    auto badParams = consensus::Main();
    badParams.nGenesisNonce = 1;
    bool threw = false;
    try {
        CreateGenesisBlock(badParams);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    return 0;
}
