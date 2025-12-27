#pragma once

#include "../../layer1-core/tx/transaction.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/validation/validation.h"
#include "../policy/policy.h"
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
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
    void SetValidationContext(const consensus::Params& params, int height, UTXOLookup lookup);
    void SetOnAccept(std::function<void(const Transaction&)> cb);

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
    std::optional<consensus::Params> m_params;
    int m_chainHeight{0};
    UTXOLookup m_lookup;
    std::function<void(const Transaction&)> m_onAccept;
    mutable std::mutex m_mutex;
};

} // namespace mempool
