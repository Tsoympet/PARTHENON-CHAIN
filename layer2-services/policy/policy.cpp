#include "policy.h"

namespace policy {

FeePolicy::FeePolicy(uint64_t minFeeRatePerKb, size_t maxTxBytes, size_t maxEntries)
    : m_minFeeRate(minFeeRatePerKb), m_maxTxBytes(maxTxBytes), m_maxEntries(maxEntries)
{
}

bool FeePolicy::IsFeeAcceptable(const Transaction& tx, uint64_t fee) const
{
    auto ser = Serialize(tx);
    if (ser.size() > m_maxTxBytes) return false;
    uint64_t required = static_cast<uint64_t>((ser.size() + 999) / 1000) * m_minFeeRate;
    return fee >= required;
}

} // namespace policy
