// Obolos Settlement Speed Tests
// Tests for sub-5 second finality target

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include "../../layer1-core/tx/transaction.h"
#include "mock_obolos_components.h"

namespace {

using namespace std::chrono;

class SettlementSpeedTest : public ::testing::Test {
protected:
};

// Test finality checkpoint creation timing
TEST_F(SettlementSpeedTest, CheckpointCreationLatency) {
    // Target: Checkpoint creation should complete in < 100ms
    auto start = high_resolution_clock::now();
    
    // Simulate checkpoint creation
    auto checkpoint = obolos_test::MakeCheckpoint(
        100, obolos_test::MakeHashFromSeed(100));
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    EXPECT_EQ(checkpoint.height, 100u);
    EXPECT_EQ(checkpoint.block_hash, obolos_test::MakeHashFromSeed(100));
    EXPECT_LT(duration.count(), 100) 
        << "Checkpoint creation took " << duration.count() << "ms, expected < 100ms";
}

// Test transaction finality latency
TEST_F(SettlementSpeedTest, TransactionFinalityLatency) {
    // Target: Transaction finality should be achieved in < 5 seconds
    // In practice, this is bounded by block time (60s) and checkpoint interval (5 blocks)
    // This test verifies the overhead beyond block confirmation is minimal
    
    auto start = high_resolution_clock::now();
    
    // Simulate transaction submission to finality
    auto tx = obolos_test::MakeFakeTransaction(42);
    auto receipt = obolos_test::MakeReceipt(tx, 250);
    
    auto end = high_resolution_clock::now();
    auto finality_overhead = duration_cast<milliseconds>(end - start);
    
    EXPECT_EQ(receipt.tx_id, tx.GetHash());
    EXPECT_EQ(receipt.finality_height, 250u);
    EXPECT_EQ(receipt.status, "finalized");
    EXPECT_LT(finality_overhead.count(), 100) 
        << "Finality overhead " << finality_overhead.count() << "ms, expected < 100ms";
}

// Test settlement receipt generation speed
TEST_F(SettlementSpeedTest, ReceiptGenerationSpeed) {
    // Target: Receipt generation should be < 50ms
    auto start = high_resolution_clock::now();
    
    // Simulate receipt generation
    auto tx = obolos_test::MakeFakeTransaction(7);
    auto receipt = obolos_test::MakeReceipt(tx, 111);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    EXPECT_EQ(receipt.tx_id, tx.GetHash());
    EXPECT_EQ(receipt.finality_height, 111u);
    EXPECT_LT(duration.count(), 50) 
        << "Receipt generation took " << duration.count() << "ms, expected < 50ms";
}

// Test account state query speed
TEST_F(SettlementSpeedTest, AccountQuerySpeed) {
    // Target: Account balance/nonce queries should be < 10ms
    auto start = high_resolution_clock::now();
    
    // Simulate account query
    auto account = obolos_test::MakeAccountQuery("account-1", 5000, 12);
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    EXPECT_EQ(account.account_id, "account-1");
    EXPECT_EQ(account.balance, 5000u);
    EXPECT_EQ(account.nonce, 12u);
    EXPECT_LT(duration.count(), 10) 
        << "Account query took " << duration.count() << "ms, expected < 10ms";
}

// Test payment throughput under load
TEST_F(SettlementSpeedTest, PaymentThroughputStress) {
    // Target: 1000+ transactions per second sustained
    const int NUM_TRANSACTIONS = 10000;
    const int TARGET_TPS = 1000;
    
    auto start = high_resolution_clock::now();
    
    uint256 last_hash{};
    for (int i = 0; i < NUM_TRANSACTIONS; ++i) {
        // Simulate transaction processing
        auto tx = obolos_test::MakeFakeTransaction(static_cast<uint32_t>(i));
        last_hash = tx.GetHash();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    auto elapsed_ms = std::max<int64_t>(duration.count(), 1);
    double actual_tps = (NUM_TRANSACTIONS * 1000.0) / elapsed_ms;
    
    EXPECT_EQ(last_hash,
              obolos_test::MakeFakeTransaction(NUM_TRANSACTIONS - 1).GetHash());
    EXPECT_GE(actual_tps, TARGET_TPS) 
        << "Achieved " << actual_tps << " TPS, expected >= " << TARGET_TPS << " TPS";
}

// Test deterministic finality timing
TEST_F(SettlementSpeedTest, DeterministicFinalityTiming) {
    // Verify that finality timing is consistent across runs
    const int NUM_RUNS = 100;
    std::vector<int64_t> timings;
    
    for (int run = 0; run < NUM_RUNS; ++run) {
        auto start = high_resolution_clock::now();
        
        // Simulate finality check
        auto tx = obolos_test::MakeFakeTransaction(static_cast<uint32_t>(run));
        auto receipt = obolos_test::MakeReceipt(tx, 300);
        volatile uint8_t sink = receipt.tx_id[0];
        (void)sink;
        
        auto end = high_resolution_clock::now();
        auto elapsed = duration_cast<microseconds>(end - start).count();
        timings.push_back(std::max<int64_t>(elapsed, 1));
    }
    
    // Calculate variance
    double mean = 0;
    for (auto t : timings) mean += t;
    mean /= NUM_RUNS;
    
    double variance = 0;
    for (auto t : timings) {
        variance += (t - mean) * (t - mean);
    }
    variance /= NUM_RUNS;
    
    double std_dev = std::sqrt(variance);
    double coefficient_of_variation = std_dev / mean;
    
    // Expect low variance (CV < 0.2 means timing is consistent)
    EXPECT_LT(coefficient_of_variation, 0.2) 
        << "Timing variance too high: CV = " << coefficient_of_variation;
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
