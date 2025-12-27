#pragma once

#include "../../layer1-core/tx/transaction.h"
#include <cstddef>

namespace policy {

class FeePolicy {
public:
    FeePolicy(uint64_t minFeeRatePerKb = 1000, size_t maxTxBytes = 100000, size_t maxEntries = 5000);

    bool IsFeeAcceptable(const Transaction& tx, uint64_t fee) const;
    size_t MaxEntries() const { return m_maxEntries; }
    uint64_t MinFeeRate() const { return m_minFeeRate; }

private:
    uint64_t m_minFeeRate; // satoshis per kB equivalent
    size_t m_maxTxBytes;
    size_t m_maxEntries;
};

} // namespace policy
