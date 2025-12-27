#include "../policy/policy.cpp"
#include "../../layer1-core/tx/transaction.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <deque>

// A minimal but deterministic mempool implementation for non-consensus relay
// purposes. It enforces a fee floor and deterministic eviction order (oldest
// first) but intentionally avoids touching consensus validation logic.
namespace mempool {

struct MempoolEntry {
    Transaction tx;
    uint64_t fee;
    std::chrono::steady_clock::time_point added;
};

class Mempool {
public:
    explicit Mempool(const policy::FeePolicy& policy)
        : m_policy(policy) {}

    bool Accept(const Transaction& tx, uint64_t fee)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        uint256 hash = tx.GetHash();
        if (m_entries.count(hash))
            return false; // duplicate
        if (!m_policy.IsFeeAcceptable(tx, fee))
            return false;
        if (m_entries.size() >= m_policy.MaxEntries())
            EvictOldest();
        m_entries.emplace(hash, MempoolEntry{tx, fee, std::chrono::steady_clock::now()});
        m_order.push_back(hash);
        return true;
    }

    bool Exists(const uint256& hash) const
    {
        std::lock_guard<std::mutex> g(m_mutex);
        return m_entries.count(hash) != 0;
    }

    std::vector<Transaction> Snapshot() const
    {
        std::lock_guard<std::mutex> g(m_mutex);
        std::vector<Transaction> out;
        out.reserve(m_entries.size());
        for (const auto& kv : m_entries)
            out.push_back(kv.second.tx);
        return out;
    }

    void Remove(const std::vector<uint256>& hashes)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        for (const auto& h : hashes) {
            auto it = m_entries.find(h);
            if (it != m_entries.end()) {
                m_entries.erase(it);
            }
        }
        // rebuild order deterministically preserving remaining items
        std::deque<uint256> rebuilt;
        for (const auto& h : m_order) {
            if (m_entries.count(h))
                rebuilt.push_back(h);
        }
        m_order.swap(rebuilt);
    }

private:
    void EvictOldest()
    {
        while (!m_order.empty()) {
            const auto hash = m_order.front();
            m_order.pop_front();
            auto it = m_entries.find(hash);
            if (it != m_entries.end()) {
                m_entries.erase(it);
                break;
            }
        }
    }

    policy::FeePolicy m_policy;
    std::unordered_map<uint256, MempoolEntry> m_entries;
    std::deque<uint256> m_order;
    mutable std::mutex m_mutex;
};

} // namespace mempool
