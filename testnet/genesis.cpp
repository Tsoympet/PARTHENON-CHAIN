#include <iomanip>
#include <iostream>

#include "../layer1-core/block/block.h"
#include "../layer1-core/consensus/genesis.cpp"
#include "../layer1-core/consensus/params.h"
#include "../layer1-core/merkle/merkle.h"

// Standalone tool to derive and print the DRACHMA testnet genesis block. It
// links directly against the consensus primitives to avoid divergence between
// documentation and implementation.

static void PrintUint256(const uint256& v)
{
    for (auto b : v)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
}

int main()
{
    const consensus::Params& params = consensus::Testnet();
    Block genesis = CreateGenesisBlock(params);

    std::cout << std::dec;
    std::cout << "Genesis time: " << params.nGenesisTime << "\n";
    std::cout << "Genesis bits: 0x" << std::hex << params.nGenesisBits << "\n";
    std::cout << "Genesis nonce: " << std::dec << genesis.header.nonce << "\n";

    std::cout << "Merkle root: 0x";
    PrintUint256(genesis.header.merkleRoot);
    std::cout << "\n";

    std::cout << "Block hash: 0x";
    PrintUint256(BlockHash(genesis.header));
    std::cout << "\n";

    return 0;
}
