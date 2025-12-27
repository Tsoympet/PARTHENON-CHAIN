#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/consensus/params.h"
#include <cassert>
#include <iostream>

int main()
{
    const auto& params = consensus::Main();
    uint32_t same = pow::CalculateNextWorkRequired(0x1e0fffff, 3600, params);
    uint32_t slow = pow::CalculateNextWorkRequired(0x1e0fffff, 4500, params);
    uint32_t fast = pow::CalculateNextWorkRequired(0x1e0fffff, 1800, params);
    assert(same == 0x1f000fff);
    assert(slow == same); // clamped to +25%
    assert(fast == 0x1f000bff);
    std::cout << "Retarget test OK\n";
    return 0;
}
