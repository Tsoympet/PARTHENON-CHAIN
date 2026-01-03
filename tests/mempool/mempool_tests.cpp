#include "../../layer2-services/mempool/mempool.h"
#include "../../layer2-services/policy/policy.h"
#include <cassert>
#include <cstdint>
#include <vector>

namespace {

Transaction MakeTx(uint8_t seed)
{
    Transaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].prevout.hash.fill(seed);
    tx.vin[0].prevout.index = seed;
    tx.vin[0].scriptSig = {seed};
    tx.vin[0].sequence = 0xffffffff;
    tx.vout[0].value = 25 + seed;
    tx.vout[0].scriptPubKey.assign(32, seed);
    return tx;
}

uint64_t RequiredFee(const policy::FeePolicy& policy, const Transaction& tx)
{
    auto ser = Serialize(tx);
    return ((ser.size() + 999) / 1000) * policy.MinFeeRate();
}

} // namespace

int main()
{
    policy::FeePolicy policy(1000, 100000, 2);
    mempool::Mempool pool(policy);

    auto tx1 = MakeTx(1);
    auto tx2 = MakeTx(2);
    auto tx3 = MakeTx(3);

    uint64_t fee1 = RequiredFee(policy, tx1);
    uint64_t fee2 = RequiredFee(policy, tx2) * 2;
    uint64_t fee3 = RequiredFee(policy, tx3) * 3;

    // Reject underpaying transactions.
    assert(!pool.Accept(tx1, fee1 - 1));
    assert(pool.Accept(tx1, fee1));
    assert(pool.Exists(tx1.GetHash()));

    // Evict lowest fee-rate when capacity exceeded.
    assert(pool.Accept(tx2, fee2));
    assert(pool.Accept(tx3, fee3));
    assert(pool.Snapshot().size() == 2);
    assert(!pool.Exists(tx1.GetHash()));
    assert(pool.Exists(tx2.GetHash()));
    assert(pool.Exists(tx3.GetHash()));

    // Removal for block payload clears entries and keeps estimates sane.
    pool.RemoveForBlock({tx2});
    assert(!pool.Exists(tx2.GetHash()));
    auto feerate = pool.EstimateFeeRate(50);
    assert(feerate >= policy.MinFeeRate());

    // Duplicate submissions and conflicts without opt-in RBF are rejected.
    assert(!pool.Accept(tx3, fee3));
    auto conflict = tx3;
    conflict.vout[0].value += 1;
    assert(!pool.Accept(conflict, fee3 + 100));
    assert(pool.Exists(tx3.GetHash()));

    // Replace-by-fee succeeds only with higher fee and replaceable flag.
    policy::FeePolicy rbfPolicy(1000, 100000, 5);
    mempool::Mempool rbfPool(rbfPolicy);
    Transaction rbfA = MakeTx(9);
    rbfA.vin[0].sequence = 0xfffffffd;
    auto feeA = RequiredFee(rbfPolicy, rbfA);
    assert(rbfPool.Accept(rbfA, feeA));
    Transaction rbfB = rbfA;
    rbfB.vout[0].value += 5;
    auto feeB = feeA + rbfPolicy.MinFeeRate();
    assert(rbfPool.Accept(rbfB, feeB));
    assert(!rbfPool.Exists(rbfA.GetHash()));
    assert(rbfPool.Exists(rbfB.GetHash()));

    // Oversize transactions fail policy checks and leave the pool empty.
    policy::FeePolicy tightPolicy(1, 10, 5);
    mempool::Mempool tight(tightPolicy);
    Transaction big = MakeTx(30);
    const uint8_t kPadByte = 0xaa; // padding to exceed tightPolicy byte limit (10 bytes)
    big.vin[0].scriptSig.assign(20, kPadByte);
    assert(!tight.Accept(big, 1000));
    assert(tight.Snapshot().empty());
    assert(tight.EstimateFeeRate(10) == tightPolicy.MinFeeRate());

    return 0;
}
