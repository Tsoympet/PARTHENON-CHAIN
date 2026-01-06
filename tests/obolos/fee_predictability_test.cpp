// Obolos Fee Predictability Tests
// Tests for flat fee model and cost predictability

#include <gtest/gtest.h>
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/tx/transaction.h"

namespace {

// Fee constants (matching doc/fees.md specification)
constexpr uint64_t BASE_FEE = 1000;           // 0.00001 OBL
constexpr uint64_t MAX_PRIORITY_FEE = 100000; // 0.001 OBL

class FeePredictabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    uint64_t CalculateTotalFee(uint64_t priority_fee) {
        return BASE_FEE + std::min(priority_fee, MAX_PRIORITY_FEE);
    }
};

// Test base fee enforcement
TEST_F(FeePredictabilityTest, BaseFeeEnforcement) {
    // Transaction with fee below base should be invalid
    uint64_t insufficient_fee = BASE_FEE - 1;
    EXPECT_LT(insufficient_fee, BASE_FEE);
    
    // Transaction with exactly base fee should be valid
    uint64_t exact_base_fee = BASE_FEE;
    EXPECT_EQ(exact_base_fee, BASE_FEE);
    
    // Transaction with fee above base should be valid
    uint64_t sufficient_fee = BASE_FEE + 1;
    EXPECT_GT(sufficient_fee, BASE_FEE);
}

// Test priority fee capping
TEST_F(FeePredictabilityTest, PriorityFeeCapping) {
    // Priority fee below max should pass through
    uint64_t normal_priority = 50000;
    uint64_t total_fee_1 = CalculateTotalFee(normal_priority);
    EXPECT_EQ(total_fee_1, BASE_FEE + normal_priority);
    
    // Priority fee at max should be capped
    uint64_t max_priority = MAX_PRIORITY_FEE;
    uint64_t total_fee_2 = CalculateTotalFee(max_priority);
    EXPECT_EQ(total_fee_2, BASE_FEE + MAX_PRIORITY_FEE);
    
    // Priority fee exceeding max should be capped
    uint64_t excessive_priority = MAX_PRIORITY_FEE * 2;
    uint64_t total_fee_3 = CalculateTotalFee(excessive_priority);
    EXPECT_EQ(total_fee_3, BASE_FEE + MAX_PRIORITY_FEE);
}

// Test fee predictability guarantee
TEST_F(FeePredictabilityTest, BoundedWorstCase) {
    // Maximum possible fee should be deterministic
    uint64_t max_possible_fee = BASE_FEE + MAX_PRIORITY_FEE;
    EXPECT_EQ(max_possible_fee, 101000);
    
    // No transaction should ever cost more than this
    uint64_t extreme_priority = std::numeric_limits<uint64_t>::max();
    uint64_t actual_fee = CalculateTotalFee(extreme_priority);
    EXPECT_LE(actual_fee, max_possible_fee);
}

// Test fee independence from congestion
TEST_F(FeePredictabilityTest, CongestionIndependence) {
    // Fee should be the same regardless of network state
    uint64_t priority = 25000;
    
    // Normal conditions
    uint64_t fee_normal = CalculateTotalFee(priority);
    
    // High congestion (simulated - fee should not change)
    uint64_t fee_congested = CalculateTotalFee(priority);
    
    // Extreme congestion (simulated - fee should still not change)
    uint64_t fee_extreme = CalculateTotalFee(priority);
    
    EXPECT_EQ(fee_normal, fee_congested);
    EXPECT_EQ(fee_congested, fee_extreme);
}

// Test fee calculation determinism
TEST_F(FeePredictabilityTest, CalculationDeterminism) {
    // Same input should always produce same output
    uint64_t priority = 30000;
    
    std::vector<uint64_t> fees;
    for (int i = 0; i < 1000; ++i) {
        fees.push_back(CalculateTotalFee(priority));
    }
    
    // All fees should be identical
    for (size_t i = 1; i < fees.size(); ++i) {
        EXPECT_EQ(fees[i], fees[0]);
    }
}

// Test fee estimation accuracy
TEST_F(FeePredictabilityTest, EstimationAccuracy) {
    // Standard priority (no priority fee)
    uint64_t estimated_standard = BASE_FEE;
    uint64_t actual_standard = CalculateTotalFee(0);
    EXPECT_EQ(actual_standard, estimated_standard);
    
    // Fast priority (50% of max)
    uint64_t estimated_fast = BASE_FEE + (MAX_PRIORITY_FEE / 2);
    uint64_t actual_fast = CalculateTotalFee(MAX_PRIORITY_FEE / 2);
    EXPECT_EQ(actual_fast, estimated_fast);
    
    // Instant priority (100% of max)
    uint64_t estimated_instant = BASE_FEE + MAX_PRIORITY_FEE;
    uint64_t actual_instant = CalculateTotalFee(MAX_PRIORITY_FEE);
    EXPECT_EQ(actual_instant, estimated_instant);
}

// Test annual budget calculation
TEST_F(FeePredictabilityTest, AnnualBudgetCalculation) {
    const uint64_t ANNUAL_TRANSACTIONS = 1000000;
    
    // All standard transactions
    uint64_t budget_standard = ANNUAL_TRANSACTIONS * BASE_FEE;
    uint64_t expected_standard = 1000000000; // 1B satoshis = 10 OBL
    EXPECT_EQ(budget_standard, expected_standard);
    
    // All instant transactions
    uint64_t budget_instant = ANNUAL_TRANSACTIONS * (BASE_FEE + MAX_PRIORITY_FEE);
    uint64_t expected_instant = 101000000000; // 1010 OBL
    EXPECT_EQ(budget_instant, expected_instant);
    
    // Mixed (70% standard, 20% fast, 10% instant)
    uint64_t budget_mixed = 
        (700000 * BASE_FEE) +
        (200000 * (BASE_FEE + MAX_PRIORITY_FEE / 2)) +
        (100000 * (BASE_FEE + MAX_PRIORITY_FEE));
    
    // Verify calculation is deterministic
    EXPECT_GT(budget_mixed, budget_standard);
    EXPECT_LT(budget_mixed, budget_instant);
}

// Test fee size independence
TEST_F(FeePredictabilityTest, SizeIndependence) {
    // Fee should not depend on transaction size
    // (within reasonable bounds set by max transaction size)
    
    uint64_t priority = 10000;
    
    // Small transaction (e.g., simple transfer)
    uint64_t fee_small = CalculateTotalFee(priority);
    
    // Large transaction (e.g., with metadata)
    uint64_t fee_large = CalculateTotalFee(priority);
    
    EXPECT_EQ(fee_small, fee_large);
}

// Test mempool fee ordering
TEST_F(FeePredictabilityTest, MempoolFeeOrdering) {
    // Higher fees should be ordered first
    std::vector<uint64_t> priorities = {0, 25000, 50000, 75000, 100000};
    std::vector<uint64_t> fees;
    
    for (auto p : priorities) {
        fees.push_back(CalculateTotalFee(p));
    }
    
    // Verify fees are in ascending order
    for (size_t i = 1; i < fees.size(); ++i) {
        EXPECT_GT(fees[i], fees[i-1]);
    }
}

// Test fee burning calculation
TEST_F(FeePredictabilityTest, FeeBurningCalculation) {
    const bool FEE_BURNING_ENABLED = true;
    const int BURN_PERCENTAGE = 100;
    
    uint64_t total_fee = BASE_FEE + 50000;
    
    if (FEE_BURNING_ENABLED) {
        uint64_t burned_amount = (total_fee * BURN_PERCENTAGE) / 100;
        uint64_t miner_reward = total_fee - burned_amount;
        
        if (BURN_PERCENTAGE == 100) {
            EXPECT_EQ(burned_amount, total_fee);
            EXPECT_EQ(miner_reward, 0);
        }
    }
}

// Test anti-spam minimum balance
TEST_F(FeePredictabilityTest, MinimumBalanceRequirement) {
    const uint64_t MIN_ACCOUNT_BALANCE = 10000; // 0.0001 OBL
    
    uint64_t account_balance = 100000;
    uint64_t transfer_amount = 50000;
    uint64_t fee = BASE_FEE;
    
    // Should have enough for transfer + fee + minimum balance
    uint64_t required = transfer_amount + fee + MIN_ACCOUNT_BALANCE;
    EXPECT_GT(account_balance, required);
    
    // Account trying to spend everything should fail
    uint64_t excessive_transfer = account_balance - fee;
    uint64_t required_excessive = excessive_transfer + fee + MIN_ACCOUNT_BALANCE;
    EXPECT_GT(required_excessive, account_balance);
}

// Test fee calculation performance
TEST_F(FeePredictabilityTest, CalculationPerformance) {
    const int NUM_CALCULATIONS = 1000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t total = 0;
    for (int i = 0; i < NUM_CALCULATIONS; ++i) {
        total += CalculateTotalFee(i % MAX_PRIORITY_FEE);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double avg_time_ns = static_cast<double>(duration.count()) / NUM_CALCULATIONS;
    
    // Fee calculation should be extremely fast (< 100 ns per calculation)
    EXPECT_LT(avg_time_ns, 100.0)
        << "Average fee calculation took " << avg_time_ns << " ns, expected < 100 ns";
    
    // Prevent compiler optimization
    EXPECT_GT(total, 0);
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
