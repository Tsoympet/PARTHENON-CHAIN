#include "mempool.h"

#include <algorithm>

namespace mempool {

Mempool::Mempool(const policy::FeePolicy& policy)
    : m_policy(policy)
{
}

bool Mempool::Accept(const Transaction& tx, uint64_t fee)
{
    std::lock_guard<std::mutex> g(m_mutex);
    const auto ser = Serialize(tx);
    const uint64_t feeRate = (ser.size() ? (fee * 1000 / ser.size()) : fee * 1000);
    uint256 hash = tx.GetHash();
    if (m_entries.count(hash)) return false;
    if (!m_policy.IsFeeAcceptable(tx, fee)) return false;

    if (m_entries.size() >= m_policy.MaxEntries()) EvictOne();

    MempoolEntry entry{tx, fee, feeRate, std::chrono::steady_clock::now()};
    m_arrival.push_back(hash);
    m_byFeeRate.emplace(feeRate, hash);
    m_entries.emplace(hash, std::move(entry));
    return true;
}

bool Mempool::Exists(const uint256& hash) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    return m_entries.count(hash) != 0;
}

std::vector<Transaction> Mempool::Snapshot() const
{
    std::lock_guard<std::mutex> g(m_mutex);
    std::vector<Transaction> out;
    out.reserve(m_entries.size());
    for (const auto& kv : m_entries) out.push_back(kv.second.tx);
    return out;
}

void Mempool::Remove(const std::vector<uint256>& hashes)
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (const auto& h : hashes) {
        auto it = m_entries.find(h);
        if (it != m_entries.end()) {
            auto range = m_byFeeRate.equal_range(it->second.feeRate);
            for (auto fr = range.first; fr != range.second; ++fr) {
                if (fr->second == h) { m_byFeeRate.erase(fr); break; }
            }
            m_entries.erase(it);
        }
    }

    std::deque<uint256> rebuilt;
    for (const auto& h : m_arrival) {
        if (m_entries.count(h)) rebuilt.push_back(h);
    }
    m_arrival.swap(rebuilt);
}

void Mempool::RemoveForBlock(const std::vector<Transaction>& blockTxs)
{
    std::vector<uint256> hashes;
    hashes.reserve(blockTxs.size());
    for (const auto& tx : blockTxs) hashes.push_back(tx.GetHash());
    Remove(hashes);
}

uint64_t Mempool::EstimateFeeRate(size_t percentile) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    if (m_entries.empty()) return m_policy.MinFeeRate();
    std::vector<uint64_t> feeRates;
    feeRates.reserve(m_entries.size());
    for (const auto& kv : m_entries) feeRates.push_back(kv.second.feeRate);
    std::sort(feeRates.begin(), feeRates.end());
    percentile = std::min(percentile, static_cast<size_t>(99));
    size_t idx = feeRates.size() * percentile / 100;
    return feeRates[idx];
}

size_t Mempool::ArrayHasher::operator()(const uint256& data) const noexcept
{
    size_t h = 0;
    for (auto b : data) h = (h * 131) ^ b;
    return h;
}

void Mempool::EvictOne()
{
    // Prefer evicting lowest feerate, break ties by oldest arrival
    if (!m_byFeeRate.empty()) {
        auto it = m_byFeeRate.begin();
        auto hash = it->second;
        m_byFeeRate.erase(it);
        m_entries.erase(hash);
        return;
    }
    while (!m_arrival.empty()) {
        auto h = m_arrival.front();
        m_arrival.pop_front();
        if (m_entries.erase(h)) break;
    }
}

} // namespace mempool
