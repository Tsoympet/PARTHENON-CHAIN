#pragma once

#include "../block/block.h"
#include <chrono>
#include <deque>
#include <optional>

// Lightweight validation anti-DoS helpers used by higher layers to prevent
// excessive validation and to bound orphan block memory usage.
class ValidationRateLimiter {
public:
    ValidationRateLimiter(uint64_t maxTokensPerMinute = 120, uint64_t burst = 240);

    // Consume a weight of validation work. Returns false when the caller should
    // defer validation because the rate limit would be exceeded.
    bool Consume(uint64_t weight = 1);

    uint64_t Tokens() const { return static_cast<uint64_t>(m_tokens); }

private:
    double m_tokens;
    double m_capacity;
    double m_refillPerSec;
    std::chrono::steady_clock::time_point m_lastRefill;

    void Refill();
};

struct OrphanBlock {
    Block block;
    uint256 hash{};
    uint256 parent{};
    std::chrono::steady_clock::time_point received;
};

class OrphanBuffer {
public:
    explicit OrphanBuffer(std::size_t maxEntries = 64);

    // Insert an orphan block; evicts the oldest when at capacity. Returns the
    // evicted orphan hash when eviction occurred.
    std::optional<uint256> Add(const OrphanBlock& orphan);

    // Remove and return all children of the provided parent hash.
    std::vector<OrphanBlock> PopChildren(const uint256& parentHash);

    std::size_t Size() const { return m_fifo.size(); }

private:
    std::size_t m_maxEntries;
    std::deque<OrphanBlock> m_fifo;
};

