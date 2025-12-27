#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../params.h"

namespace consensus {

enum class ThresholdState {
    DEFINED,
    STARTED,
    LOCKED_IN,
    ACTIVE,
    FAILED
};

struct BlockVersionSample {
    int height;
    int64_t time;
    int32_t version;
};

uint32_t VersionBitsMask(const VBDeployment& deployment);
bool VersionBitsSignal(int32_t version, const VBDeployment& deployment);

// Returns the state for the tip described by the provided history.
ThresholdState VersionBitsState(
    const Params& params,
    const VBDeployment& deployment,
    const std::vector<BlockVersionSample>& history);

// Computes a block version that includes all deployments that are permitted to
// signal at the provided median time.
uint32_t ComputeBlockVersion(
    const Params& params,
    const std::vector<VBDeployment>& deployments,
    int64_t medianTimePast);

} // namespace consensus
