#include "../../layer1-core/consensus/params.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    const auto& params = consensus::Main();
    uint64_t supply = 0;
    for (int h = 0; h < 800000; ++h) {
        supply += consensus::GetBlockSubsidy(h, params);
        assert(consensus::MoneyRange(supply, params));
    }
    assert(supply <= consensus::GetMaxMoney(params));
    std::cout << "Supply test OK\n";
    return 0;
}
