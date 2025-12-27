#include "fork_resolution.h"
#include <algorithm>
#include <stdexcept>

namespace consensus {

ForkResolver::ForkResolver(uint32_t finalizationDepth, uint32_t reorgWorkMarginBps)
    : m_finalizationDepth(finalizationDepth), m_reorgMarginBps(reorgWorkMarginBps)
{
    if (m_reorgMarginBps == 0)
        m_reorgMarginBps = 1; // prevent divide-by-zero
}

bool ForkResolver::ConsiderHeader(const BlockHeader& header, const uint256& hash, const uint256& parentHash, uint32_t height, const Params& params)
{
    (void)params; // Params kept for future rule toggles
    auto blockWork = ChainWork(powalgo::CalculateBlockWork(header.bits));
    ChainWork cumulative = blockWork;

    if (!std::all_of(parentHash.begin(), parentHash.end(), [](uint8_t b) { return b == 0; })) {
        auto it = m_index.find(parentHash);
        if (it == m_index.end())
            throw std::runtime_error("parent header unknown to fork resolver");
        cumulative += it->second.chainWork;
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

std::vector<uint256> ForkResolver::ReorgPath(const uint256& newTip) const
{
    std::vector<uint256> path;
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

} // namespace consensus

