#include <gtest/gtest.h>
#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/consensus/params.h"

TEST(PoW, ProofOfWorkBoundaries)
{
    auto params = consensus::Main();
    uint256 zero{};
    EXPECT_TRUE(powalgo::CheckProofOfWork(zero, params.nGenesisBits, params));

    uint32_t invalidBits = params.nGenesisBits | 0x01000000;
    EXPECT_FALSE(powalgo::CheckProofOfWork(zero, invalidBits, params));

    EXPECT_THROW(powalgo::CalculateBlockWork(0x00800000), std::runtime_error);
}

TEST(PoW, DifficultyClampsTimespan)
{
    consensus::Params params = consensus::Main();
    params.nPowTargetTimespan = 100;
    uint32_t last = params.nGenesisBits;
    // actualTimespan below 50 will be clamped to 50
    auto tightened = powalgo::CalculateNextWorkRequired(last, 1, params);
    auto relaxed = powalgo::CalculateNextWorkRequired(last, 1000, params);
    EXPECT_NE(tightened, relaxed);
    EXPECT_GT(relaxed, tightened);
}

TEST(PoW, WorkIncreasesWithDifficulty)
{
    auto params = consensus::Main();
    auto easy = powalgo::CalculateBlockWork(0x207fffff);
    auto baseline = powalgo::CalculateBlockWork(params.nGenesisBits);
    EXPECT_GT(baseline, 0);
    EXPECT_LT(easy, baseline);
}
