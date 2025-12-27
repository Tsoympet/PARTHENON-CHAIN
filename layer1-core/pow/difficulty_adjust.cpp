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
    return std::clamp(span, targetTimespan / 2, targetTimespan * 2);
}

RetargetMetrics CalculateAdaptiveNextWork(
    const std::vector<uint32_t>& timestamps,
    const std::vector<uint32_t>& bits,
    const consensus::Params& params)
{
    if (timestamps.size() < 3 || bits.size() != timestamps.size())
        throw std::runtime_error("retarget window too small or inconsistent");

    // Take the median of the last three timestamps to blunt miner timestamp games.
    const size_t n = timestamps.size();
    std::array<uint32_t,3> last{{timestamps[n-3], timestamps[n-2], timestamps[n-1]}};
    std::sort(last.begin(), last.end());
    uint32_t median = last[1];

    // Use a simple exponential moving average to gently react to variance without
    // opening the door to wild oscillations. Weight recent blocks slightly more.
    int64_t weightedSpan = 0;
    int64_t weightSum = 0;
    for (size_t i = 1; i < n; ++i) {
        int64_t delta = static_cast<int64_t>(timestamps[i]) - static_cast<int64_t>(timestamps[i-1]);
        int64_t weight = static_cast<int64_t>(i + 1); // newest blocks carry more weight
        weightedSpan += delta * weight;
        weightSum += weight;
    }
    int64_t averageTimespan = weightedSpan / std::max<int64_t>(1, weightSum);
    int64_t medianSpacing = static_cast<int64_t>(timestamps.back()) - static_cast<int64_t>(median);
    if (medianSpacing > 0)
        averageTimespan = (averageTimespan + medianSpacing) / 2;
    averageTimespan = ClampRetargetTimespan(averageTimespan, params);

    // Anchor difficulty to the median block from the window to avoid reorg-based
    // attacks that exploit a single outlier block.
    uint32_t anchorBits = bits[n / 2];
    uint32_t next = CalculateNextWorkRequired(anchorBits, averageTimespan, params);

    // Damp changes so each retarget step cannot exceed +/-25% unless the network
    // is dramatically off schedule.
    const auto powLimitWork = CalculateBlockWork(params.nGenesisBits);
    auto anchorWork = CalculateBlockWork(anchorBits);
    auto proposedWork = CalculateBlockWork(next);
    auto upperBound = anchorWork + (anchorWork / 4);
    auto lowerBound = anchorWork - (anchorWork / 4);
    if (proposedWork > upperBound && upperBound < powLimitWork)
        next = CalculateNextWorkRequired(anchorBits, params.nPowTargetTimespan + params.nPowTargetTimespan / 4, params);
    else if (proposedWork < lowerBound)
        next = CalculateNextWorkRequired(anchorBits, params.nPowTargetTimespan - params.nPowTargetTimespan / 4, params);

    return RetargetMetrics{next, averageTimespan, anchorBits};
}

} // namespace powalgo
