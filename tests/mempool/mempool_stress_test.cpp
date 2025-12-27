#include <gtest/gtest.h>
#include "../../layer2-services/mempool/mempool.h"
#include "../../layer2-services/policy/policy.h"

static Transaction MakeTx(uint8_t seed, uint64_t value)
{
    Transaction tx;
    TxIn in;
    in.prevout.hash.fill(seed);
    in.prevout.index = seed;
    tx.vin.push_back(in);

    TxOut out;
    out.value = value;
    out.scriptPubKey = {0x51};
    tx.vout.push_back(out);
    return tx;
}

TEST(MempoolStress, EvictsLowestFeeWhenFull)
{
    policy::FeePolicy policy(/*minFeeRate*/1, /*maxTxBytes*/100000, /*maxEntries*/5);
    mempool::Mempool pool(policy);

    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(pool.Accept(MakeTx(static_cast<uint8_t>(i), 10 + i), 100 + i));
    }
    // Now insert higher fee, should evict something and still accept
    EXPECT_TRUE(pool.Accept(MakeTx(9, 20), 1000));
    auto snapshot = pool.Snapshot();
    EXPECT_EQ(snapshot.size(), 5u);
    // Ensure highest fee spend made it in and duplicate spend rejected
    EXPECT_TRUE(pool.Exists(MakeTx(9, 20).GetHash()));
    EXPECT_FALSE(pool.Accept(MakeTx(9, 30), 1));
}

TEST(MempoolStress, FeeRateEstimateTracksPercentile)
{
    policy::FeePolicy policy(/*minFeeRate*/1, /*maxTxBytes*/100000, /*maxEntries*/10);
    mempool::Mempool pool(policy);
    for (int i = 0; i < 10; ++i) {
        pool.Accept(MakeTx(static_cast<uint8_t>(i + 50), 5), 10 + i * 10);
    }
    auto median = pool.EstimateFeeRate(50);
    EXPECT_GE(median, policy.MinFeeRate());
    EXPECT_LT(pool.EstimateFeeRate(90), pool.EstimateFeeRate(99));
}
