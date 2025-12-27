#pragma once

#include "params.h"
#include "../block/block.h"
#include "../pow/difficulty.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

namespace consensus {

struct ChainWork {
    boost::multiprecision::cpp_int value;

    ChainWork() : value(0) {}
    explicit ChainWork(const boost::multiprecision::cpp_int& v) : value(v) {}

    ChainWork& operator+=(const ChainWork& other)
    {
        value += other.value;
        return *this;
    }
};

struct BlockMeta {
    uint256 hash{};
    uint256 parent{};
    uint32_t height{0};
    uint32_t time{0};
    uint32_t bits{0};
    ChainWork chainWork{};
};

struct Uint256Hasher {
    std::size_t operator()(const uint256& h) const noexcept
    {
        std::size_t acc = 0;
        for (auto b : h)
            acc = (acc * 131) ^ b;
        return acc;
    }
};

struct Uint256Eq {
    bool operator()(const uint256& a, const uint256& b) const noexcept
    {
        return std::equal(a.begin(), a.end(), b.begin());
    }
};

// Maintains best-chain selection with safeguards that make deep reorganizations
// difficult unless the competing fork has clearly superior cumulative work.
class ForkResolver {
public:
    explicit ForkResolver(uint32_t finalizationDepth = 100, uint32_t reorgWorkMarginBps = 500);

    // Returns true if the incoming header became the new tip.
    bool ConsiderHeader(const BlockHeader& header, const uint256& hash, const uint256& parentHash, uint32_t height, const Params& params);

    const BlockMeta* Tip() const { return m_bestTip ? &(*m_bestTip) : nullptr; }
    std::vector<uint256> ReorgPath(const uint256& newTip) const;

private:
    uint32_t m_finalizationDepth;
    uint32_t m_reorgMarginBps; // 10_000 = 100%
    std::unordered_map<uint256, BlockMeta, Uint256Hasher, Uint256Eq> m_index;
    std::optional<BlockMeta> m_bestTip;

    bool IsBetterChain(const BlockMeta& candidate) const;
    bool ViolatesCheckpoint(uint32_t height, const uint256& hash, const Params& params) const;
};

} // namespace consensus

