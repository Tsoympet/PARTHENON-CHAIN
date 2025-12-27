#include <gtest/gtest.h>
#include "../../layer2-services/net/p2p.h"

TEST(P2P, BloomFilterMatch)
{
    net::BloomFilter full;
    full.full = true;
    uint256 any{};
    EXPECT_TRUE(full.Match(any));

    net::BloomFilter empty;
    empty.bits.assign(4, 0x00);
    empty.full = false;
    empty.nHashFuncs = 2;
    empty.tweak = 0;
    EXPECT_FALSE(empty.Match(any));

    net::BloomFilter permissive;
    permissive.bits.assign(4, 0xFF);
    permissive.full = false;
    permissive.nHashFuncs = 2;
    permissive.tweak = 1;
    any[0] = 0x42;
    EXPECT_TRUE(permissive.Match(any));
}
