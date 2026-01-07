#include "mempool.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace mempool {

Mempool::Mempool(const policy::FeePolicy& policy)
    : m_policy(policy)
{
    m_lookup = [](const OutPoint&) { return std::optional<TxOut>{}; };
}

bool Mempool::MaybeReplace(const Transaction& tx, uint64_t fee, uint64_t feeRate)
{
    // Simple RBF: require every input to already be spent by a mempool tx and higher fee rate
    std::vector<uint256> conflicts;
    for (const auto& in : tx.vin) {
        auto it = m_spent.find(in.prevout);
        if (it == m_spent.end()) return false; // not replaceable
        conflicts.push_back(it->second);
    }

    // ensure all conflicts signal replaceability
    for (const auto& h : conflicts) {
        auto entIt = m_entries.find(h);
        if (entIt == m_entries.end() || !entIt->second.replaceable) return false;
        if (feeRate <= entIt->second.feeRate) return false;
    }

    Remove(conflicts);
    return true;
}

bool Mempool::Accept(const Transaction& tx, uint64_t fee)
{
    std::function<void(const Transaction&)> callback;
    Transaction txCopy;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        const auto ser = Serialize(tx);
        const size_t txSize = ser.size();
        const uint64_t feeRate = (txSize ? (fee * 1000 / txSize) : fee * 1000);
        uint256 hash = tx.GetHash();
        if (m_entries.count(hash)) return false;
        if (!m_policy.IsFeeAcceptable(tx, fee)) return false;

        if (m_params) {
            std::vector<Transaction> batch{tx};
            if (!ValidateTransactions(batch, *m_params, m_chainHeight, m_lookup)) return false;
        }

        bool replace = false;
        for (const auto& in : tx.vin) {
            if (in.sequence < 0xfffffffe) { replace = true; break; }
        }
        for (const auto& in : tx.vin) {
            if (m_spent.count(in.prevout)) {
                if (!MaybeReplace(tx, fee, feeRate)) return false;
                replace = true;
                break;
            }
        }

        if (m_entries.size() >= m_policy.MaxEntries()) EvictOne();
        EvictExpired();

        MempoolEntry entry{tx, fee, feeRate, txSize, std::chrono::steady_clock::now(), replace};
        m_arrival.push_back(hash);
        m_byFeeRate.emplace(feeRate, hash);
        m_entries.emplace(hash, std::move(entry));
        for (const auto& in : tx.vin) m_spent[in.prevout] = hash;
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
    out.reserve(m_entries.size()); // Pre-allocate to avoid reallocations
    
    // Build a vector of (hash, transaction) pairs to avoid recomputing hashes during sort
    std::vector<std::pair<uint256, Transaction>> pairs;
    pairs.reserve(m_entries.size());
    for (const auto& kv : m_entries) {
        pairs.emplace_back(kv.first, kv.second.tx); // Use entry's hash key directly
    }
    
    // Sort by pre-computed hash
    std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    
    // Extract transactions in sorted order
    for (auto& p : pairs) {
        out.push_back(std::move(p.second));
    }
    return out;
}

void Mempool::Remove(const std::vector<uint256>& hashes)
{
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
    std::lock_guard<std::mutex> g(m_mutex);
    Remove(hashes);
}

uint64_t Mempool::EstimateFeeRate(size_t percentile) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    if (m_entries.empty()) return m_policy.MinFeeRate();
    
    percentile = std::clamp(percentile, static_cast<size_t>(1), static_cast<size_t>(99));
    
    // m_byFeeRate is already sorted ascending, work directly with it
    const size_t totalSize = m_byFeeRate.size();
    if (totalSize == 0) return m_policy.MinFeeRate();
    
    // Calculate position with interpolation
    const double pos = (percentile / 100.0) * (totalSize - 1);
    const size_t lowerIdx = static_cast<size_t>(pos);
    
    if (lowerIdx >= totalSize - 1) {
        // Return the highest fee rate
        return m_byFeeRate.rbegin()->first;
    }
    
    // Optimize: single iteration instead of two std::next calls
    auto it = m_byFeeRate.begin();
    std::advance(it, lowerIdx);
    const uint64_t lower = it->first;
    ++it;
    const uint64_t upper = it->first;
    
    // Linear interpolation between two closest values
    const double fraction = pos - lowerIdx;
    return static_cast<uint64_t>(lower + fraction * (upper - lower));
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

void Mempool::EvictExpired()
{
    auto now = std::chrono::steady_clock::now();
    const auto maxAge = std::chrono::hours(72);
    std::vector<uint256> expired;
    for (const auto& kv : m_entries) {
        if (now - kv.second.added > maxAge) expired.push_back(kv.first);
    }
    if (!expired.empty()) Remove(expired);

    size_t approxSize = 0;
    for (const auto& kv : m_entries) approxSize += kv.second.txSize;
    while (approxSize > m_targetBytes && !m_byFeeRate.empty()) {
        auto victim = m_byFeeRate.begin()->second;
        auto entryIt = m_entries.find(victim);
        size_t vsize = 0;
        if (entryIt != m_entries.end()) vsize = entryIt->second.txSize;
        Remove({victim});
        if (approxSize >= vsize) approxSize -= vsize; else break;
    }
}

} // namespace mempool

