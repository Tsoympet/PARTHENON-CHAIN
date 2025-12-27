#pragma once

#include "../../layer1-core/tx/transaction.h"
#include "../policy/policy.h"
#include <chrono>
#include <cstddef>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mempool {

struct MempoolEntry {
    Transaction tx;
    uint64_t fee{0};
    uint64_t feeRate{0};
    std::chrono::steady_clock::time_point added;
};

class Mempool {
public:
    explicit Mempool(const policy::FeePolicy& policy);

    bool Accept(const Transaction& tx, uint64_t fee);
    bool Exists(const uint256& hash) const;
    bool SpendsKnown(const OutPoint& op) const;
    std::vector<Transaction> Snapshot() const;
    void Remove(const std::vector<uint256>& hashes);
    void RemoveForBlock(const std::vector<Transaction>& blockTxs);
    uint64_t EstimateFeeRate(size_t percentile) const; // sat/kB

private:
    struct ArrayHasher {
        size_t operator()(const uint256& data) const noexcept;
    };

    struct OutPointHasher {
        size_t operator()(const OutPoint& op) const noexcept;
    };

    struct OutPointEqual {
        bool operator()(const OutPoint& a, const OutPoint& b) const noexcept;
    };

    void EvictOne();

    policy::FeePolicy m_policy;
    std::unordered_map<uint256, MempoolEntry, ArrayHasher> m_entries;
    std::multimap<uint64_t, uint256> m_byFeeRate; // feeRate -> txid
    std::deque<uint256> m_arrival;
    std::unordered_map<OutPoint, uint256, OutPointHasher, OutPointEqual> m_spent;
    mutable std::mutex m_mutex;
};

} // namespace mempool
