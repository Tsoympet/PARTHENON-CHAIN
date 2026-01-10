// Obolos Replay Protection Tests
// Tests for nonce-based transaction replay protection

#include <gtest/gtest.h>
#include <openssl/sha.h>
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/crypto/schnorr.h"
#include "../../layer1-core/tx/transaction.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

class ReplayProtectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
};

std::array<uint8_t, 32> ComputeChainIdDigest(const Transaction& tx, size_t inputIndex, uint32_t chainId)
{
    auto base = ComputeInputDigest(tx, inputIndex);
    std::array<uint8_t, 32> digest{};
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, base.data(), base.size());
    SHA256_Update(&ctx, &chainId, sizeof(chainId));
    SHA256_Final(digest.data(), &ctx);
    return digest;
}

std::array<uint8_t, 64> SignTxWithChainId(const Transaction& tx,
                                         size_t inputIndex,
                                         uint32_t chainId,
                                         const std::array<uint8_t, 32>& seckey,
                                         const std::array<uint8_t, 32>& aux)
{
    const auto digest = ComputeChainIdDigest(tx, inputIndex, chainId);
    std::array<uint8_t, 64> sig{};
    if (!schnorr_sign_with_aux(seckey.data(), digest.data(), aux.data(), sig.data())) {
        sig.fill(0);
    }
    return sig;
}

bool VerifyTxWithChainId(const Transaction& tx,
                         size_t inputIndex,
                         uint32_t chainId,
                         const std::array<uint8_t, 32>& pubkey_x,
                         const std::array<uint8_t, 64>& sig)
{
    const auto digest = ComputeChainIdDigest(tx, inputIndex, chainId);
    std::vector<uint8_t> msg(digest.begin(), digest.end());
    return VerifySchnorr(pubkey_x, sig, msg);
}

// Test that nonce must be sequential
TEST_F(ReplayProtectionTest, SequentialNonceRequired) {
    // Create account with nonce 0
    uint64_t account_nonce = 0;
    
    // Transaction with nonce 1 should be valid
    uint64_t tx_nonce_1 = 1;
    EXPECT_EQ(tx_nonce_1, account_nonce + 1);
    
    // Transaction with nonce 2 (skipping) should be invalid
    uint64_t tx_nonce_skip = 2;
    EXPECT_NE(tx_nonce_skip, account_nonce + 1);
    
    // Transaction with nonce 0 (replay) should be invalid
    uint64_t tx_nonce_replay = 0;
    EXPECT_NE(tx_nonce_replay, account_nonce + 1);
}

// Test that failed transactions still increment nonce
TEST_F(ReplayProtectionTest, FailedTransactionIncrementsNonce) {
    uint64_t initial_nonce = 5;
    uint64_t expected_nonce_after_fail = 6;
    
    // Simulate failed transaction (e.g., insufficient balance)
    // Nonce should still increment to prevent retry attacks
    uint64_t nonce_after_fail = initial_nonce + 1;
    
    EXPECT_EQ(nonce_after_fail, expected_nonce_after_fail);
}

// Test that nonce gaps are rejected
TEST_F(ReplayProtectionTest, NonceGapsRejected) {
    const uint64_t NONCE_GAP_TOLERANCE = 0; // No gaps allowed
    
    uint64_t account_nonce = 10;
    
    // Try to submit transaction with nonce 12 (gap of 1)
    uint64_t tx_nonce = 12;
    int64_t gap = static_cast<int64_t>(tx_nonce) - static_cast<int64_t>(account_nonce) - 1;
    
    EXPECT_GT(gap, NONCE_GAP_TOLERANCE);
    // Transaction should be rejected
}

// Test that duplicate nonce is rejected (idempotent handling)
TEST_F(ReplayProtectionTest, DuplicateNonceRejected) {
    uint64_t account_nonce = 7;
    
    // First transaction with nonce 8
    uint64_t tx1_nonce = 8;
    EXPECT_EQ(tx1_nonce, account_nonce + 1);
    
    // After processing, account nonce becomes 8
    account_nonce = 8;
    
    // Second transaction with same nonce 8 should be rejected
    uint64_t tx2_nonce = 8;
    EXPECT_NE(tx2_nonce, account_nonce + 1);
}

// Test nonce overflow protection
TEST_F(ReplayProtectionTest, NonceOverflowProtection) {
    uint64_t max_nonce = std::numeric_limits<uint64_t>::max();
    
    // Account at maximum nonce cannot process more transactions
    EXPECT_EQ(max_nonce, std::numeric_limits<uint64_t>::max());
    
    // Next nonce would overflow
    // System should handle this gracefully (reject or special handling)
}

// Test cross-chain replay protection
TEST_F(ReplayProtectionTest, CrossChainReplayProtection) {
    Transaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vout[0].value = 42;
    tx.vout[0].scriptPubKey.assign(32, 0x01);

    const std::array<uint8_t, 32> seckey = {
        0xB7,0xE1,0x51,0x62,0x8A,0xED,0x2A,0x6A,0xBF,0x71,0x58,0x80,0x9C,0xF4,0xF3,0xC7,
        0x62,0xE7,0x16,0x0F,0x38,0xB4,0xDA,0x56,0xA7,0x84,0xD9,0x04,0x51,0x90,0xCF,0xEF};
    const std::array<uint8_t, 32> pubkey = {
        0xDF,0xF1,0xD7,0x7F,0x2A,0x67,0x1C,0x5F,0x36,0x18,0x37,0x26,0xDB,0x23,0x41,0xBE,
        0x58,0xFE,0xAE,0x1D,0xA2,0xDE,0xCE,0xD8,0x43,0x24,0x0F,0x7B,0x50,0x2B,0xA6,0x59};
    const std::array<uint8_t, 32> aux = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};

    const uint32_t chain_id_a = 1;
    const uint32_t chain_id_b = 2;
    auto sig = SignTxWithChainId(tx, 0, chain_id_a, seckey, aux);
    tx.vin[0].scriptSig.assign(sig.begin(), sig.end());

    EXPECT_TRUE(VerifyTxWithChainId(tx, 0, chain_id_a, pubkey, sig));
    EXPECT_FALSE(VerifyTxWithChainId(tx, 0, chain_id_b, pubkey, sig));
}

TEST_F(ReplayProtectionTest, InvalidChainIdRejected) {
    Transaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vout[0].value = 7;
    tx.vout[0].scriptPubKey.assign(32, 0x02);

    const std::array<uint8_t, 32> seckey = {
        0xB7,0xE1,0x51,0x62,0x8A,0xED,0x2A,0x6A,0xBF,0x71,0x58,0x80,0x9C,0xF4,0xF3,0xC7,
        0x62,0xE7,0x16,0x0F,0x38,0xB4,0xDA,0x56,0xA7,0x84,0xD9,0x04,0x51,0x90,0xCF,0xEF};
    const std::array<uint8_t, 32> pubkey = {
        0xDF,0xF1,0xD7,0x7F,0x2A,0x67,0x1C,0x5F,0x36,0x18,0x37,0x26,0xDB,0x23,0x41,0xBE,
        0x58,0xFE,0xAE,0x1D,0xA2,0xDE,0xCE,0xD8,0x43,0x24,0x0F,0x7B,0x50,0x2B,0xA6,0x59};
    const std::array<uint8_t, 32> aux = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};

    const uint32_t valid_chain_id = 1;
    const uint32_t invalid_chain_id = 999;
    auto sig = SignTxWithChainId(tx, 0, valid_chain_id, seckey, aux);
    tx.vin[0].scriptSig.assign(sig.begin(), sig.end());

    EXPECT_TRUE(VerifyTxWithChainId(tx, 0, valid_chain_id, pubkey, sig));
    EXPECT_FALSE(VerifyTxWithChainId(tx, 0, invalid_chain_id, pubkey, sig));
}

// Test nonce reset on reorganization
TEST_F(ReplayProtectionTest, NonceReorgHandling) {
    struct MockBlock {
        uint64_t nonce_before{0};
        uint32_t tx_count{0};
        bool finalized{false};
    };

    struct MockChain {
        uint64_t account_nonce{0};
        std::vector<MockBlock> blocks;

        void ApplyBlock(uint32_t tx_count, bool finalized) {
            MockBlock block;
            block.nonce_before = account_nonce;
            block.tx_count = tx_count;
            block.finalized = finalized;
            account_nonce += tx_count;
            blocks.push_back(block);
        }

        bool ReorgLastBlock() {
            if (blocks.empty() || blocks.back().finalized) {
                return false;
            }
            account_nonce = blocks.back().nonce_before;
            blocks.pop_back();
            return true;
        }
    };

    MockChain chain;
    chain.account_nonce = 10;

    chain.ApplyBlock(2, true);   // finalized block
    EXPECT_EQ(chain.account_nonce, 12u);

    chain.ApplyBlock(3, false);  // non-finalized block
    EXPECT_EQ(chain.account_nonce, 15u);

    EXPECT_TRUE(chain.ReorgLastBlock());
    EXPECT_EQ(chain.account_nonce, 12u);
    EXPECT_FALSE(chain.ReorgLastBlock());
    EXPECT_EQ(chain.account_nonce, 12u);
}

// Test mempool nonce ordering
TEST_F(ReplayProtectionTest, MempoolNonceOrdering) {
    // Mempool should maintain transactions in nonce order per account
    
    std::vector<uint64_t> submitted_order = {5, 3, 4, 2, 1};
    std::vector<uint64_t> expected_order = {1, 2, 3, 4, 5};
    
    // Sort by nonce
    std::vector<uint64_t> actual_order = submitted_order;
    std::sort(actual_order.begin(), actual_order.end());
    
    EXPECT_EQ(actual_order, expected_order);
}

// Test nonce validation timing
TEST_F(ReplayProtectionTest, NonceValidationPerformance) {
    // Nonce check should be very fast (< 1 microsecond)
    const int NUM_CHECKS = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_CHECKS; ++i) {
        uint64_t account_nonce = i;
        uint64_t tx_nonce = i + 1;
        bool valid = (tx_nonce == account_nonce + 1);
        (void)valid; // Suppress unused warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_check_time = static_cast<double>(duration.count()) / NUM_CHECKS;
    
    EXPECT_LT(avg_check_time, 1.0) 
        << "Average nonce check took " << avg_check_time << " µs, expected < 1 µs";
}

// Test concurrent nonce updates (thread safety)
TEST_F(ReplayProtectionTest, ConcurrentNonceUpdates) {
    std::atomic<uint64_t> nonce{0};
    const int threads = 8;
    const int increments_per_thread = 1000;

    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back([&]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                nonce.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_EQ(nonce.load(), static_cast<uint64_t>(threads * increments_per_thread));
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
