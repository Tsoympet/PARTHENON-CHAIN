#include <gtest/gtest.h>
#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/consensus/params.h"

TEST(Difficulty, ClampsExtremeTimespans)
{
    const auto& params = consensus::Main();
    // Use lastBits from genesis for repeatability
    uint32_t base = params.nGenesisBits;
    int64_t veryFast = params.nPowTargetTimespan / 10; // should clamp to /2
    int64_t verySlow = params.nPowTargetTimespan * 10; // should clamp to *2

    uint32_t fast = powalgo::CalculateNextWorkRequired(base, veryFast, params);
    uint32_t slow = powalgo::CalculateNextWorkRequired(base, verySlow, params);

    EXPECT_NE(fast, 0u);
    EXPECT_NE(slow, 0u);
    // New work should still remain within the pow limit and move in expected direction
    EXPECT_LE(fast, base);
    EXPECT_GE(slow, base);
}

TEST(Difficulty, RejectsOverflowTargets)
{
    const auto& params = consensus::Main();
    uint256 hash{};
    hash.fill(0xFF);
    // Pow limit from params.nGenesisBits should reject higher target
    EXPECT_FALSE(powalgo::CheckProofOfWork(hash, params.nGenesisBits - 1, params));
}
