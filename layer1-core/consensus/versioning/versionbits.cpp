#include "versionbits.h"
#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>

namespace consensus {

uint32_t VersionBitsMask(const VBDeployment& deployment)
{
    if (deployment.bit < 0 || deployment.bit > 28)
        throw std::invalid_argument("Version bits deployment bit out of range");
    return 1u << deployment.bit;
}

bool VersionBitsSignal(int32_t version, const VBDeployment& deployment)
{
    return (version & VersionBitsMask(deployment)) != 0;
}

static int64_t MedianTime(const std::vector<BlockVersionSample>& window)
{
    if (window.empty()) return 0;

    std::vector<int64_t> times;
    times.reserve(window.size());
    for (const auto& entry : window)
        times.push_back(entry.time);

    std::sort(times.begin(), times.end());
    size_t mid = times.size() / 2;
    if (times.size() % 2)
        return times[mid];
    return (times[mid - 1] + times[mid]) / 2;
}

ThresholdState VersionBitsState(
    const Params& params,
    const VBDeployment& deployment,
    const std::vector<BlockVersionSample>& history)
{
    if (history.empty())
        return ThresholdState::DEFINED;

    std::vector<BlockVersionSample> sortedHistory = history;
    std::sort(sortedHistory.begin(), sortedHistory.end(), [](const auto& a, const auto& b){ return a.height < b.height; });

    int period = static_cast<int>(params.nMinerConfirmationWindow);
    int threshold = static_cast<int>(params.nRuleChangeActivationThreshold);
    int currentHeight = sortedHistory.back().height;
    int currentPeriod = (currentHeight + 1) / period;

    ThresholdState state = ThresholdState::DEFINED;
    for (int p = 0; p <= currentPeriod; ++p) {
        int periodStart = p * period;
        int periodEnd = (p + 1) * period - 1;

        std::vector<BlockVersionSample> window;
        for (const auto& entry : sortedHistory) {
            if (entry.height >= periodStart && entry.height <= periodEnd)
                window.push_back(entry);
        }

        int64_t mtp = MedianTime(window);

        switch (state) {
            case ThresholdState::DEFINED:
                if (mtp >= deployment.nTimeout) {
                    state = ThresholdState::FAILED;
                } else if (mtp >= deployment.nStartTime) {
                    state = ThresholdState::STARTED;
                }
                break;
            case ThresholdState::STARTED:
                if (mtp >= deployment.nTimeout) {
                    state = ThresholdState::FAILED;
                    break;
                }
                {
                    int signals = 0;
                    for (const auto& entry : window) {
                        if (VersionBitsSignal(entry.version, deployment))
                            ++signals;
                    }
                    if (signals >= threshold)
                        state = ThresholdState::LOCKED_IN;
                }
                break;
            case ThresholdState::LOCKED_IN:
                state = ThresholdState::ACTIVE;
                break;
            case ThresholdState::ACTIVE:
            case ThresholdState::FAILED:
                break;
        }
    }

    return state;
}

uint32_t ComputeBlockVersion(
    const Params& params,
    const std::vector<VBDeployment>& deployments,
    int64_t medianTimePast)
{
    (void)params; // Reserved for future consensus use such as custom thresholds.
    uint32_t version = 0x20000000; // Base version as per BIP9-style signaling

    for (const auto& dep : deployments) {
        if (dep.bit < 0)
            continue;
        if (dep.nStartTime == -1)
            continue; // disabled
        if (medianTimePast < dep.nStartTime)
            continue;
        if (dep.nTimeout != -1 && medianTimePast >= dep.nTimeout)
            continue;
        version |= VersionBitsMask(dep);
    }

    // Enforce that required top bits are set to avoid misuse of legacy version fields.
    version |= 0x10000000;
    return version;
}

} // namespace consensus
