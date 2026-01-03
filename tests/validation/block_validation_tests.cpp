#include "../../layer1-core/validation/validation.h"
#include "../../layer1-core/merkle/merkle.h"
#include "../../layer1-core/consensus/params.h"
#include <cassert>
#include <cstdint>
#include <limits>

namespace {

consensus::Params LooseParams()
{
    consensus::Params p = consensus::Testnet();
    p.nGenesisBits = 0x207fffff; // easiest possible target to smooth tests
    p.fPowAllowMinDifficultyBlocks = true;
    return p;
}

OutPoint NullPrevout()
{
    OutPoint op{};
    op.index = std::numeric_limits<uint32_t>::max();
    op.hash.fill(0);
    return op;
}

Transaction MakeCoinbase(uint64_t value)
{
    Transaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].prevout = NullPrevout();
    tx.vin[0].scriptSig = {0x01, 0x02};
    tx.vin[0].sequence = 0xffffffff;
    tx.vout[0].value = value;
    tx.vout[0].scriptPubKey.assign(32, 0x01);
    tx.vin[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    tx.vout[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    return tx;
}

} // namespace

int main()
{
    const auto params = LooseParams();

    // Block header future-drift and median time past enforcement.
    {
        BlockHeader h{};
        h.bits = params.nGenesisBits;
        h.time = 2000;
        BlockValidationOptions opts;
        opts.medianTimePast = 2100;
        assert(!ValidateBlockHeader(h, params, opts));

        opts.medianTimePast = 1500;
        opts.now = 1500;
        opts.maxFutureDrift = 10;
        h.time = 1520;
        assert(!ValidateBlockHeader(h, params, opts));

        h.time = 1505;
        assert(ValidateBlockHeader(h, params, opts));
    }

    // Fully validate a single-coinbase block with Merkle root and PoW limits.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 1600;
        block.header.version = 1;
        block.transactions.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params, static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);
        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        assert(ValidateBlock(block, params, 1, {}, opts));
    }

    // Reject when Merkle root mismatches transactions.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 1700;
        block.header.version = 1;
        block.transactions.push_back(MakeCoinbase(consensus::GetBlockSubsidy(2, params, static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot.fill(0xFF);
        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        assert(!ValidateBlock(block, params, 2, {}, opts));
    }

    // Enforce nft_state_root anchoring when requested by the caller.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 1800;
        block.header.version = 1;
        const int height = 3;
        block.transactions.push_back(MakeCoinbase(
            consensus::GetBlockSubsidy(height, params,
                                       static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);

        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        opts.requireNftStateRoot = true;

        // Missing anchor should be rejected.
        assert(!ValidateBlock(block, params, height, {}, opts));

        // Mismatched anchor should be rejected.
        opts.nftStateRoot.fill(0x01);
        opts.expectedNftStateRoot.fill(0x02);
        assert(!ValidateBlock(block, params, height, {}, opts));

        // Matching, non-zero anchor should succeed.
        opts.expectedNftStateRoot = opts.nftStateRoot;
        assert(ValidateBlock(block, params, height, {}, opts));
    }

    return 0;
}
