#include <gtest/gtest.h>
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/merkle/merkle.h"
#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/validation/validation.h"
#include <limits>

namespace {

Block MakePrivateBlock(const uint256& prev, uint32_t time, const consensus::Params& params)
{
    Block block;
    block.header.version = 1;
    block.header.prevBlockHash = prev;
    block.header.time = time;
    block.header.bits = params.nGenesisBits;
    block.header.nonce = 0;

    Transaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.index = std::numeric_limits<uint32_t>::max();
    coinbase.vin[0].scriptSig = {0x00, 0x01};
    coinbase.vout.push_back({50 * 100'000'000ULL, std::vector<uint8_t>(32, 0x99)});

    block.transactions.push_back(coinbase);
    block.header.merkleRoot = ComputeMerkleRoot(block.transactions);

    // Quickly grind a valid nonce under the very easy regtest target so header validation passes.
    while (!powalgo::CheckProofOfWork(BlockHash(block.header), block.header.bits, params)) {
        ++block.header.nonce;
    }
    return block;
}

} // namespace

TEST(Integration, PrivateRegtestChainProgression)
{
    consensus::Params params = consensus::Main();
    params.fPowAllowMinDifficultyBlocks = true;
    params.nPowTargetSpacing = 10;
    params.nPowTargetTimespan = 100;
    params.nGenesisBits = 0x207fffff; // regtest-style easy target

    uint32_t startTime = 1700000000;
    Block genesis = MakePrivateBlock(uint256{}, startTime, params);

    BlockValidationOptions opts{};
    opts.medianTimePast = startTime - 1;
    opts.now = startTime;

    ASSERT_TRUE(ValidateBlock(genesis, params, /*height=*/0, {}, opts));
    uint256 tipHash = BlockHash(genesis.header);

    // Extend the chain twice to ensure monotonic time and merkle validity.
    for (int height = 1; height <= 2; ++height) {
        uint32_t t = startTime + params.nPowTargetSpacing * static_cast<uint32_t>(height);
        Block next = MakePrivateBlock(tipHash, t, params);
        opts.medianTimePast = t - 1;
        opts.now = t;
        ASSERT_TRUE(ValidateBlock(next, params, height, {}, opts));
        tipHash = BlockHash(next.header);
    }
}
