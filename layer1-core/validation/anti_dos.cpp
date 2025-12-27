#include "anti_dos.h"
#include <algorithm>

ValidationRateLimiter::ValidationRateLimiter(uint64_t maxTokensPerMinute, uint64_t burst)
    : m_tokens(static_cast<double>(burst)),
      m_capacity(static_cast<double>(burst)),
      m_refillPerSec(static_cast<double>(maxTokensPerMinute) / 60.0),
      m_lastRefill(std::chrono::steady_clock::now())
{
    if (m_capacity == 0) m_capacity = 1;
    if (m_refillPerSec == 0) m_refillPerSec = 1;
}

void ValidationRateLimiter::Refill()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - m_lastRefill).count();
    m_lastRefill = now;
    m_tokens = std::min(m_capacity, m_tokens + elapsed * m_refillPerSec);
}

bool ValidationRateLimiter::Consume(uint64_t weight)
{
    Refill();
    if (weight > static_cast<uint64_t>(m_capacity))
        return false; // refuse obviously abusive single requests
    if (m_tokens < static_cast<double>(weight))
        return false;
    m_tokens -= static_cast<double>(weight);
    return true;
}

OrphanBuffer::OrphanBuffer(std::size_t maxEntries)
    : m_maxEntries(maxEntries)
{
    if (m_maxEntries == 0)
        m_maxEntries = 1; // keep buffer usable
}

std::optional<uint256> OrphanBuffer::Add(const OrphanBlock& orphan)
{
    std::optional<uint256> evicted;
    if (m_fifo.size() >= m_maxEntries) {
        evicted = m_fifo.front().hash;
        m_fifo.pop_front();
    }
    m_fifo.push_back(orphan);
    return evicted;
}

std::vector<OrphanBlock> OrphanBuffer::PopChildren(const uint256& parentHash)
{
    std::vector<OrphanBlock> ready;
    auto it = m_fifo.begin();
    while (it != m_fifo.end()) {
        if (it->parent == parentHash) {
            ready.push_back(*it);
            it = m_fifo.erase(it);
        } else {
            ++it;
        }
    }
    return ready;
}

