#include "params.h"
#include <algorithm>
#include <limits>

namespace {
constexpr uint64_t COIN = 100000000ULL;
constexpr uint32_t DEFAULT_WINDOW = 2016;                 // ~33.6 hours at 60s blocks
constexpr uint32_t DEFAULT_THRESHOLD = (DEFAULT_WINDOW * 95) / 100; // 95% threshold
}

namespace consensus {

static Params mainParams {
    210000,            // halving interval
    60,                // 60 sec block time
    3600,              // 1 hour retarget window
    60,                // 60 blocks per retarget interval
    42000000ULL * COIN,
    false,             // no min-difficulty blocks
    1735689600,        // genesis time
    0x1e0fffff,        // initial difficulty
    0,                 // mined later if not provided
    "DRACHMA: Proof before trust",
    DEFAULT_THRESHOLD,
    DEFAULT_WINDOW,
    { VBDeployment{28, -1, -1} }
};

static Params testParams {
    210000,
    60,
    3600,
    60,
    42000000ULL * COIN,
    true,              // allow min difficulty
    1735689600,
    0x1f00ffff,
    0,
    "DRACHMA TESTNET",
    DEFAULT_THRESHOLD,
    DEFAULT_WINDOW,
    { VBDeployment{28, -1, -1} }
};

const Params& Main()    { return mainParams; }
const Params& Testnet() { return testParams; }

uint64_t GetBlockSubsidy(int height, const Params& params)
{
    if (height < 0) return 0;

    int halvings = height / static_cast<int>(params.nSubsidyHalvingInterval);
    if (halvings >= 64) // protect against shift overflow
        return 0;

    uint64_t subsidy = 50 * COIN;
    subsidy >>= halvings;
    return subsidy;
}

uint64_t GetMaxMoney(const Params& params)
{
    return params.nMaxMoneyOut;
}

bool MoneyRange(uint64_t amount, const Params& params)
{
    return amount <= params.nMaxMoneyOut;
}

} // namespace consensus
