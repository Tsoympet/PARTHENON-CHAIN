#include "../../layer1-core/merkle/merkle.h"
#include "../../layer1-core/tx/transaction.h"
#include <cassert>
#include <iostream>

int main()
{
    Transaction a;
    a.vin.push_back({OutPoint{uint256{}, 0xffffffff}, std::vector<uint8_t>{'c','o','i','n','b','a','s','e'}, 0xffffffff});
    a.vout.push_back({50, std::vector<uint8_t>{'p','a','y','-','t','o','-','p','u','b','k','e','y'}});

    Transaction b;
    OutPoint prev{}; prev.hash.fill(0x01); prev.index = 0;
    b.vin.push_back({prev, std::vector<uint8_t>{'s','p','e','n','d'}, 1});
    b.vout.push_back({25, std::vector<uint8_t>{'p','a','y','2'}});

    std::vector<Transaction> txs{a, b};
    auto root = ComputeMerkleRoot(txs);
    const uint8_t expected[32] = {0x15,0xe3,0xc0,0x48,0x27,0x0c,0x7e,0x5a,0x3c,0x78,0xb6,0xcc,0xe7,0x5d,0xce,0x6c,
                                  0xa8,0xae,0xe4,0xdb,0xd8,0x07,0x02,0xcf,0xc3,0x96,0x98,0x0f,0x69,0xc0,0x39,0x94};
    assert(std::equal(root.begin(), root.end(), expected));
    std::cout << "Merkle test OK\n";
    return 0;
}
