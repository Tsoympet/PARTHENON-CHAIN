#include "difficulty_adjust.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace powalgo {

int64_t ClampRetargetTimespan(int64_t span, const consensus::Params& params)
{
    if (params.nPowTargetTimespan == 0)
        throw std::runtime_error("targetTimespan cannot be zero");
    int64_t targetTimespan = params.nPowTargetTimespan;
    // Bitcoin-style hard bounds: 1/4x to 4x the expected window.
    return std::clamp(span, targetTimespan / 4, targetTimespan * 4);
}

RetargetMetrics CalculateAdaptiveNextWork(
    const std::vector<uint32_t>& timestamps,
    const std::vector<uint32_t>& bits,
    const consensus::Params& params)
{
    if (timestamps.size() < params.nDifficultyAdjustmentInterval || bits.size() != timestamps.size())
        throw std::runtime_error("retarget window too small or inconsistent");

    // Use the first and last block of the window (2016 for Bitcoin-style
    // scheduling) to measure actual timespan. Timestamps are assumed ordered
    // oldest->newest.
    const size_t n = timestamps.size();
    int64_t actualTimespan = static_cast<int64_t>(timestamps.back()) - static_cast<int64_t>(timestamps.front());
    int64_t targetTimespan = static_cast<int64_t>(params.nPowTargetTimespan);

    // Dampening as per Bitcoin Cash DAA: move 1/4 of the distance toward the
    // observed timespan to reduce volatility while remaining responsive.
    int64_t dampened = targetTimespan + (actualTimespan - targetTimespan) / 4;
    dampened = ClampRetargetTimespan(dampened, params);

    // Emergency difficulty drop: if the most recent block is far in the past,
    // allow minimum difficulty to keep the chain moving on small networks.
    if (params.fPowAllowMinDifficultyBlocks && n >= 2) {
        if (timestamps.back() > timestamps[n - 2] &&
            timestamps.back() - timestamps[n - 2] > params.nPowTargetSpacing * 2) {
            return RetargetMetrics{params.nGenesisBits, dampened, bits.back()};
        }
    }

    const uint32_t anchorBits = bits.back();
    uint32_t next = CalculateNextWorkRequired(anchorBits, dampened, params);

    return RetargetMetrics{next, dampened, anchorBits};
}

} // namespace powalgo
