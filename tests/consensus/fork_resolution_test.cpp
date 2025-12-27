#include "../../layer1-core/consensus/fork_resolution.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/block/block.h"
#include <cassert>
#include <cstring>
#include <algorithm>

namespace {

BlockHeader MakeHeader(const uint256& prev, uint32_t time, uint32_t bits)
{
    BlockHeader h{};
    h.version = 1;
    h.prevBlockHash = prev;
    h.merkleRoot.fill(0);
    h.time = time;
    h.bits = bits;
    h.nonce = 0;
    return h;
}

} // namespace

int main()
{
    auto params = consensus::Main();
    consensus::ForkResolver resolver(/*finalizationDepth=*/2, /*reorgWorkMarginBps=*/500);

    uint256 nullHash{};
    auto genesisHeader = MakeHeader(nullHash, params.nGenesisTime, params.nGenesisBits);
    auto genesisHash = BlockHash(genesisHeader);
    assert(resolver.ConsiderHeader(genesisHeader, genesisHash, nullHash, 0, params));

    auto b1 = MakeHeader(genesisHash, genesisHeader.time + 1, params.nGenesisBits);
    auto h1 = BlockHash(b1);
    assert(resolver.ConsiderHeader(b1, h1, genesisHash, 1, params));

    auto b2 = MakeHeader(h1, b1.time + 1, params.nGenesisBits);
    auto h2 = BlockHash(b2);
    assert(resolver.ConsiderHeader(b2, h2, h1, 2, params));

    auto b3 = MakeHeader(h2, b2.time + 1, params.nGenesisBits);
    auto h3 = BlockHash(b3);
    assert(resolver.ConsiderHeader(b3, h3, h2, 3, params));

    // Alternate fork starting at height 1 with much higher difficulty (more work
    // per block) should only replace the tip when the cumulative work margin is
    // met despite being a deeper reorg than finalizationDepth.
    uint32_t tougherBits = params.nGenesisBits - 0x010000; // synthetic extra difficulty
    auto alt1 = MakeHeader(genesisHash, b1.time + 5, tougherBits);
    auto altH1 = BlockHash(alt1);
    resolver.ConsiderHeader(alt1, altH1, genesisHash, 1, params);

    auto alt2 = MakeHeader(altH1, alt1.time + 1, tougherBits);
    auto altH2 = BlockHash(alt2);
    resolver.ConsiderHeader(alt2, altH2, altH1, 2, params);

    auto alt3 = MakeHeader(altH2, alt2.time + 1, tougherBits);
    auto altH3 = BlockHash(alt3);
    bool becameTip = resolver.ConsiderHeader(alt3, altH3, altH2, 3, params);

    // The higher-work fork should displace the original chain.
    assert(becameTip);
    assert(resolver.Tip());
    assert(std::equal(resolver.Tip()->hash.begin(), resolver.Tip()->hash.end(), altH3.begin()));

    // The reorg path should trace genesis -> alt1 -> alt2 -> alt3
    auto path = resolver.ReorgPath(altH3);
    assert(path.size() == 4);
    assert(std::equal(path.front().begin(), path.front().end(), genesisHash.begin()));

    // Hardened checkpoint should reject conflicting headers at the pinned height.
    params.checkpoints[1] = h1;
    consensus::ForkResolver checkpointed(/*finalizationDepth=*/2, /*reorgWorkMarginBps=*/500);
    assert(checkpointed.ConsiderHeader(genesisHeader, genesisHash, nullHash, 0, params));
    assert(checkpointed.ConsiderHeader(b1, h1, genesisHash, 1, params));
    auto bad = MakeHeader(genesisHash, b1.time + 10, params.nGenesisBits);
    auto badHash = BlockHash(bad);
    assert(!checkpointed.ConsiderHeader(bad, badHash, genesisHash, 1, params));

    return 0;
}

