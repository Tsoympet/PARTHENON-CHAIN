#include <gtest/gtest.h>
#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/pow/sha256d.h"
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

TEST(PoW, Sha256dRejectsNullInputs)
{
    uint8_t out[32]{};
    sha256d(out, nullptr, 0);
    // Zero length with valid buffer should leave deterministic hash of empty string.
    Hash256 empty = SHA256d(nullptr, 0);
    EXPECT_NE(empty, Hash256{});

    // Null output buffer should be ignored safely.
    sha256d(nullptr, reinterpret_cast<const uint8_t*>("a"), 1);
}

TEST(PoW, CheckPowDisallowsEqualTarget)
{
    uint256 target{};
    target.fill(0x01);
    uint8_t hash[32]{};
    std::fill(std::begin(hash), std::end(hash), 0x01);
    // When hash equals target it should fail the strict < comparison.
    EXPECT_FALSE(check_pow(hash, target));
}
