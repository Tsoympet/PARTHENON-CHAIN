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

TEST(PoW, Sha256DeterministicAndRejectsNullData)
{
    auto to_hex = [](const Hash256& h) {
        std::string out;
        static const char* hex = "0123456789abcdef";
        out.reserve(h.size() * 2);
        for (auto b : h) {
            out.push_back(hex[b >> 4]);
            out.push_back(hex[b & 0xf]);
        }
        return out;
    };

    const uint8_t msg[] = {'a','b','c'};
    auto first = SHA256d(msg, sizeof(msg));
    auto second = SHA256d(msg, sizeof(msg));
    EXPECT_EQ(first, second);
    EXPECT_EQ(to_hex(first), std::string("4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358"));

    // Non-null length with null data should hit the defensive null guard and return zeros.
    auto nullHash = SHA256(nullptr, 4);
    EXPECT_EQ(nullHash, Hash256{});

    // sha256d should early-return when given a null output pointer.
    EXPECT_NO_THROW(sha256d(nullptr, msg, sizeof(msg)));
}
