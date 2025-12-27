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
    const uint8_t expected[32] = {0xf7,0x2d,0xba,0x20,0xf5,0xd6,0x46,0x4a,0xd5,0xa0,0xe5,0x64,0x17,0x72,0xaf,0x5b,
                                  0x41,0xd6,0x46,0xa1,0xfd,0x5d,0x35,0x8f,0xed,0x5b,0x36,0x19,0x93,0x4c,0xdc,0x6f};
    assert(std::equal(root.begin(), root.end(), expected));
    std::cout << "Merkle test OK\n";
    return 0;
}
