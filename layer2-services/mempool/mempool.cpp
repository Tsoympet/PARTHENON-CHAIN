#include "mempool.h"

#include <algorithm>
#include <optional>

namespace mempool {

Mempool::Mempool(const policy::FeePolicy& policy)
    : m_policy(policy)
{
    m_lookup = [](const OutPoint&) { return std::optional<TxOut>{}; };
}

bool Mempool::Accept(const Transaction& tx, uint64_t fee)
{
    std::function<void(const Transaction&)> callback;
    Transaction txCopy;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        const auto ser = Serialize(tx);
        const uint64_t feeRate = (ser.size() ? (fee * 1000 / ser.size()) : fee * 1000);
        uint256 hash = tx.GetHash();
        if (m_entries.count(hash)) return false;
        if (!m_policy.IsFeeAcceptable(tx, fee)) return false;

        if (m_params) {
            std::vector<Transaction> batch{tx};
            if (!ValidateTransactions(batch, *m_params, m_chainHeight, m_lookup)) return false;
        }

        for (const auto& in : tx.vin) {
            if (m_spent.count(in.prevout)) return false; // double spend
        }

        if (m_entries.size() >= m_policy.MaxEntries()) EvictOne();

        MempoolEntry entry{tx, fee, feeRate, std::chrono::steady_clock::now()};
        m_arrival.push_back(hash);
        m_byFeeRate.emplace(feeRate, hash);
        m_entries.emplace(hash, std::move(entry));
        for (const auto& in : tx.vin) m_spent.emplace(in.prevout, hash);
        callback = m_onAccept;
        txCopy = tx;
    }
    if (callback) callback(txCopy);
    return true;
}

bool Mempool::Exists(const uint256& hash) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    return m_entries.count(hash) != 0;
}

bool Mempool::SpendsKnown(const OutPoint& op) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    return m_spent.count(op) != 0;
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
            for (const auto& in : it->second.tx.vin) {
                auto s = m_spent.find(in.prevout);
                if (s != m_spent.end() && s->second == h) m_spent.erase(s);
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

void Mempool::SetValidationContext(const consensus::Params& params, int height, UTXOLookup lookup)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_params = params;
    m_chainHeight = height;
    m_lookup = std::move(lookup);
}

void Mempool::SetOnAccept(std::function<void(const Transaction&)> cb)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_onAccept = std::move(cb);
}

size_t Mempool::ArrayHasher::operator()(const uint256& data) const noexcept
{
    size_t h = 0;
    for (auto b : data) h = (h * 131) ^ b;
    return h;
}

size_t Mempool::OutPointHasher::operator()(const OutPoint& op) const noexcept
{
    size_t h = 0;
    for (auto b : op.hash) h = (h * 131) ^ b;
    h ^= static_cast<size_t>(op.index + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    return h;
}

bool Mempool::OutPointEqual::operator()(const OutPoint& a, const OutPoint& b) const noexcept
{
    return a.index == b.index && a.hash == b.hash;
}

void Mempool::EvictOne()
{
    // Prefer evicting lowest feerate, break ties by oldest arrival
    if (!m_byFeeRate.empty()) {
        auto it = m_byFeeRate.begin();
        auto hash = it->second;
        m_byFeeRate.erase(it);
        auto entryIt = m_entries.find(hash);
        if (entryIt != m_entries.end()) {
            for (const auto& in : entryIt->second.tx.vin) {
                auto s = m_spent.find(in.prevout);
                if (s != m_spent.end() && s->second == hash) m_spent.erase(s);
            }
            m_entries.erase(entryIt);
        }
        return;
    }
    while (!m_arrival.empty()) {
        auto h = m_arrival.front();
        m_arrival.pop_front();
        auto it = m_entries.find(h);
        if (it != m_entries.end()) {
            for (const auto& in : it->second.tx.vin) {
                auto s = m_spent.find(in.prevout);
                if (s != m_spent.end() && s->second == h) m_spent.erase(s);
            }
            m_entries.erase(it);
            break;
        }
    }
}

} // namespace mempool
