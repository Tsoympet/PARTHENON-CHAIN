#include "params.h"

namespace consensus {

static Params mainParams {
    210000,            // halving interval
    60,                // 60 sec block time
    3600,              // 1 hour retarget window
    60,                // 60 blocks
    42000000ULL * 100000000ULL,
    false,             // no min-difficulty blocks
    1735689600,        // genesis time
    0x1e0fffff,        // initial difficulty
    0,                 // mined later
    "DRACHMA: Proof before trust"
};

static Params testParams {
    210000,
    60,
    3600,
    60,
    42000000ULL * 100000000ULL,
    true,              // allow min difficulty
    1735689600,
    0x1f00ffff,
    0,
    "DRACHMA TESTNET"
};

const Params& Main()    { return mainParams; }
const Params& Testnet() { return testParams; }

} // namespace consensus
