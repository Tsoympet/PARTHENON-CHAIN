#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/consensus/params.h"
#include <cassert>
#include <iostream>

int main()
{
    const auto& params = consensus::Main();
    uint32_t same = powalgo::CalculateNextWorkRequired(0x1e0fffff, 3600, params);
    uint32_t slow = powalgo::CalculateNextWorkRequired(0x1e0fffff, 4500, params);
    uint32_t fast = powalgo::CalculateNextWorkRequired(0x1e0fffff, 1800, params);
    uint32_t extremeSlow = powalgo::CalculateNextWorkRequired(0x1e0fffff, 3600 * 10, params);

    assert(slow >= same); // slower blocks lower difficulty (higher nBits)
    assert(fast <= same); // faster blocks raise difficulty (lower nBits)
    assert(extremeSlow >= slow); // clamped to 2x window but still loosens difficulty
    std::cout << "Retarget test OK\n";
    return 0;
}
