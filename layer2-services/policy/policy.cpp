#include "../../layer1-core/tx/transaction.h"
#include <cstddef>
#include <vector>

namespace policy {

class FeePolicy {
public:
    FeePolicy(uint64_t minFeeRatePerKb = 1000, size_t maxTxBytes = 100000, size_t maxEntries = 5000)
        : m_minFeeRate(minFeeRatePerKb), m_maxTxBytes(maxTxBytes), m_maxEntries(maxEntries) {}

    bool IsFeeAcceptable(const Transaction& tx, uint64_t fee) const
    {
        auto ser = Serialize(tx);
        if (ser.size() > m_maxTxBytes)
            return false;
        uint64_t required = static_cast<uint64_t>((ser.size() + 999) / 1000) * m_minFeeRate;
        return fee >= required;
    }

    size_t MaxEntries() const { return m_maxEntries; }

private:
    uint64_t m_minFeeRate; // satoshis per kB equivalent
    size_t m_maxTxBytes;
    size_t m_maxEntries;
};

} // namespace policy
