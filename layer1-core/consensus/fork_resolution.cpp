#include "fork_resolution.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace consensus {

ForkResolver::ForkResolver(uint32_t finalizationDepth, uint32_t reorgWorkMarginBps)
    : m_finalizationDepth(finalizationDepth), m_reorgMarginBps(reorgWorkMarginBps)
{
    if (m_reorgMarginBps == 0)
        m_reorgMarginBps = 1; // prevent divide-by-zero
}

bool ForkResolver::ConsiderHeader(const BlockHeader& header, const uint256& hash, const uint256& parentHash, uint32_t height, const Params& params, uint32_t now, uint32_t maxFutureDrift)
{
    std::lock_guard<std::mutex> l(m_mu);
    if (ViolatesCheckpoint(height, hash, params))
        return false;

    // Reject known-invalid hashes early.
    if (m_invalid.find(hash) != m_invalid.end())
        return false;

    bool becameTip = AttachAndUpdateTip(header, hash, parentHash, height, params, now, maxFutureDrift);
    ProcessOrphans(hash, params, now, maxFutureDrift);
    return becameTip;
}

bool ForkResolver::ViolatesCheckpoint(uint32_t height, const uint256& hash, const Params& params) const
{
    auto it = params.checkpoints.find(height);
    if (it == params.checkpoints.end())
        return false;
    return !std::equal(it->second.begin(), it->second.end(), hash.begin());
}

std::vector<uint256> ForkResolver::ReorgPath(const uint256& newTip) const
{
    std::lock_guard<std::mutex> l(m_mu);
    std::vector<uint256> path;
    // Pre-allocate for typical chain depth to avoid reallocations
    path.reserve(16);
    auto it = m_index.find(newTip);
    while (it != m_index.end()) {
        path.push_back(it->second.hash);
        if (std::all_of(it->second.parent.begin(), it->second.parent.end(), [](uint8_t b) { return b == 0; }))
            break;
        it = m_index.find(it->second.parent);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

bool ForkResolver::IsBetterChain(const BlockMeta& candidate) const
{
    if (!m_bestTip)
        return true;

    const auto& current = *m_bestTip;
    if (candidate.chainWork.value <= current.chainWork.value)
        return false;

    // Harden against majority attacks by requiring significantly more work for
    // deep reorganizations. Shallow reorganizations (within the finalization
    // window) follow the standard most-work rule.
    if (candidate.height + m_finalizationDepth >= current.height)
        return true;

    auto required = current.chainWork.value * (10000 + m_reorgMarginBps) / 10000;
    return candidate.chainWork.value > required;
}

uint32_t ForkResolver::ComputeMedianTimePast(const uint256& parent) const
{
    std::vector<uint32_t> times;
    times.reserve(11);

    auto it = m_index.find(parent);
    while (it != m_index.end() && times.size() < 11) {
        times.push_back(it->second.time);
        if (std::all_of(it->second.parent.begin(), it->second.parent.end(), [](uint8_t b) { return b == 0; }))
            break;
        it = m_index.find(it->second.parent);
    }

    if (times.empty())
        return 0;
    std::sort(times.begin(), times.end());
    return times[times.size() / 2];
}

bool ForkResolver::AttachAndUpdateTip(const BlockHeader& header, const uint256& hash, const uint256& parentHash, uint32_t height, const Params& params, uint32_t now, uint32_t maxFutureDrift)
{
    bool hasParent = std::all_of(parentHash.begin(), parentHash.end(), [](uint8_t b) { return b == 0; }) == false;
    ChainWork cumulative = ChainWork(powalgo::CalculateBlockWork(header.bits));

    if (hasParent) {
        auto it = m_index.find(parentHash);
        if (it == m_index.end()) {
            // Parent unknown: stash as orphan and revisit later.
            m_orphans[parentHash].push_back(OrphanBlock{header, hash, parentHash, height});
            return false;
        }
        cumulative += it->second.chainWork;
    }

    uint32_t medianTimePast = hasParent ? ComputeMedianTimePast(parentHash) : 0;
    if (medianTimePast != 0 && header.time <= medianTimePast) {
        m_invalid[hash] = "timestamp-below-median";
        return false;
    }
    uint64_t horizon = static_cast<uint64_t>(now) + maxFutureDrift;
    uint32_t clampedHorizon = horizon > std::numeric_limits<uint32_t>::max()
        ? std::numeric_limits<uint32_t>::max()
        : static_cast<uint32_t>(horizon);
    if (header.time > clampedHorizon) {
        m_invalid[hash] = "timestamp-too-new";
        return false;
    }

    BlockMeta meta{hash, parentHash, height, header.time, header.bits, cumulative};
    m_index[hash] = meta;

    if (!m_bestTip) {
        m_bestTip = meta;
        return true;
    }

    if (!IsBetterChain(meta))
        return false;

    m_bestTip = meta;
    return true;
}

void ForkResolver::ProcessOrphans(const uint256& parentHash, const Params& params, uint32_t now, uint32_t maxFutureDrift)
{
    auto it = m_orphans.find(parentHash);
    if (it == m_orphans.end())
        return;

    auto pending = it->second;
    m_orphans.erase(it);

    for (const auto& orphan : pending) {
        AttachAndUpdateTip(orphan.header, orphan.hash, orphan.parent, orphan.height, params, now, maxFutureDrift);
        ProcessOrphans(orphan.hash, params, now, maxFutureDrift);
    }
}

} // namespace consensus

