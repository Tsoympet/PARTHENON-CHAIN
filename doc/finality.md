# Finality Guarantees

## Overview

PARTHENON CHAIN implements different finality models depending on the asset and use case. This document specifies the finality guarantees for each asset and the mechanisms used to achieve deterministic settlement.

## Finality Models

### Probabilistic Finality (TLN, DRM)

Talanton (TLN) and Drachma (DRM) use a traditional Proof-of-Work consensus with probabilistic finality.

**Characteristics:**
- **Finality Type:** Probabilistic
- **Confirmation Time:** 6+ confirmations (~6 minutes)
- **Reorganization Risk:** Decreases exponentially with confirmations
- **Practical Finality:** 60 confirmations (~60 minutes) for high-value transactions
- **Mechanism:** Cumulative proof-of-work

**Security Model:**
- Reorganization probability: `P(reorg) ≈ (q/p)^k` where:
  - `k` = number of confirmations
  - `q` = attacker hashrate fraction
  - `p` = honest hashrate fraction

**Use Cases:**
- Standard cryptocurrency transfers
- Smart contract deployment (DRM)
- NFT anchoring (TLN)

### Deterministic Finality (OBL)

Obolos (OBL) implements deterministic finality through checkpoint-based settlement.

**Characteristics:**
- **Finality Type:** Deterministic
- **Target Finality Time:** < 5 seconds
- **Reorganization Risk:** Zero after finality
- **Practical Finality:** Immediate upon checkpoint confirmation
- **Mechanism:** Finality checkpoints + consensus rules

**Security Model:**
- Once finalized, transactions are irreversible
- Nodes reject reorganizations conflicting with finalized state
- Checkpoint creation requires PoW consensus

**Use Cases:**
- Institutional payments
- High-frequency settlement
- Cross-border transfers
- Payment channel settlement

## Obolos Finality Mechanism

### Checkpoint Creation

Finality checkpoints are created at regular intervals:

```
CreateFinalityCheckpoint(block_height):
    if block_height % FINALITY_CHECKPOINT_INTERVAL != 0:
        return None
    
    checkpoint = {
        block_height: block_height,
        block_hash: GetBlockHash(block_height),
        state_root: ComputeAccountStateRoot(),
        prev_checkpoint: GetPreviousCheckpoint(),
        timestamp: GetBlockTime(block_height)
    }
    
    checkpoint.hash = Hash(checkpoint)
    return checkpoint
```

**Parameters:**
- `FINALITY_CHECKPOINT_INTERVAL = 5 blocks` (~5 minutes at 60s target)
- Checkpoints are consensus-critical and validated by all nodes

### Finality Rules

1. **Checkpoint Validation:**
   - Checkpoint must be at valid height (`height % INTERVAL == 0`)
   - State root must match computed account state
   - Block hash must be valid
   - Previous checkpoint must be valid

2. **Reorganization Constraints:**
   - Nodes MUST NOT reorganize past the latest finality checkpoint
   - Competing chains with conflicting finalized checkpoints are rejected
   - Finalized blocks are immutable

3. **Transaction Finality:**
   - Transaction is **pending** if in mempool
   - Transaction is **confirmed** if in a block
   - Transaction is **finalized** if block contains a finality checkpoint or is before one

### Finality Proof

Clients can obtain cryptographic proof of finality:

```
FinalityProof {
    // Transaction being proven
    transaction: ObolosTransaction
    transaction_index: uint32
    
    // Block containing transaction
    block_height: uint64
    block_hash: bytes32
    block_timestamp: uint64
    
    // Merkle proof of transaction inclusion
    merkle_proof: MerkleProof
    
    // Finality checkpoint
    checkpoint: FinalityCheckpoint
    checkpoint_height: uint64
    
    // Proof that checkpoint covers this block
    coverage_proof: bytes
}
```

**Verification:**
```
VerifyFinalityProof(proof):
    // 1. Verify transaction is in block
    if !VerifyMerkleProof(proof.merkle_proof, proof.transaction, proof.block_hash):
        return false
    
    // 2. Verify checkpoint is valid
    if !VerifyCheckpoint(proof.checkpoint):
        return false
    
    // 3. Verify checkpoint covers this block
    if proof.block_height > proof.checkpoint_height:
        return false
    
    // 4. Verify checkpoint is on canonical chain
    if !IsCheckpointCanonical(proof.checkpoint):
        return false
    
    return true
```

### Settlement Receipt

Every finalized transaction has an immutable settlement receipt:

```
SettlementReceipt {
    // Transaction identity
    transaction_id: bytes32
    transaction_hash: bytes32
    
    // Settlement details
    sender: address
    receiver: address
    amount: uint64
    fee: uint64
    reference_id: bytes32
    
    // Timing
    block_height: uint64
    block_timestamp: uint64
    finality_timestamp: uint64
    
    // Status
    status: enum {
        Pending,      // In mempool
        Confirmed,    // In block, not finalized
        Finalized,    // Past finality checkpoint
        Failed        // Validation failed
    }
    
    // Proofs
    finality_proof: FinalityProof
    state_proof: StateProof
}
```

## Finality Invariants

The finality mechanism maintains strict invariants:

### Invariant 1: Non-Reversibility

**Statement:** Finalized transactions cannot be reversed.

**Enforcement:**
- Nodes reject reorganizations affecting finalized checkpoints
- Finality checkpoints are consensus-enforced
- Checkpoint conflicts result in chain rejection

### Invariant 2: Deterministic Ordering

**Statement:** Finalized transactions have immutable ordering.

**Enforcement:**
- Nonce mechanism ensures deterministic transaction ordering
- Checkpoint records exact block height and position
- Transaction order cannot change after finality

### Invariant 3: State Consistency

**Statement:** Finalized state transitions are consistent across all nodes.

**Enforcement:**
- State root in checkpoint matches computed state
- All nodes compute identical state transitions
- State proof verifies account balance transitions

### Invariant 4: Bounded Finality Time

**Statement:** Finality is achieved within bounded time.

**Target:** < 5 seconds average finality time

**Enforcement:**
- Checkpoints created every 5 blocks (~5 minutes)
- Block time target: 60 seconds
- Fast block propagation minimizes confirmation delay

### Invariant 5: Idempotence

**Statement:** Duplicate finalization requests are safely ignored.

**Enforcement:**
- Nonce prevents transaction replay
- Checkpoint uniqueness enforced by height
- Identical settlement receipts for same transaction

## Performance Characteristics

### Finality Latency

Expected finality times under normal operation:

| Scenario | Expected Time | Worst Case |
|----------|--------------|------------|
| Single block confirmation | ~60 seconds | ~120 seconds |
| Checkpoint creation | ~5 minutes | ~10 minutes |
| End-to-end finality | ~6 minutes | ~12 minutes |
| Light client verification | ~10 seconds | ~30 seconds |

### Throughput Impact

Finality mechanism overhead:

- **Checkpoint Storage:** ~200 bytes per checkpoint
- **Proof Size:** ~1 KB per transaction proof
- **Verification Time:** < 10ms per proof
- **State Root Computation:** ~100ms per checkpoint

### Network Impact

- **Checkpoint Propagation:** < 1 second
- **Proof Request/Response:** < 100ms
- **Receipt Generation:** < 50ms per transaction

## Comparison with Other Systems

| System | Finality Type | Time | Reversibility | Use Case |
|--------|--------------|------|---------------|----------|
| Bitcoin | Probabilistic | ~60 min | Exponentially decreasing | Store of value |
| Ethereum PoW | Probabilistic | ~12 min | Exponentially decreasing | Smart contracts |
| Ethereum PoS | Deterministic | ~15 min | Zero after finality | Smart contracts |
| Algorand | Immediate | ~5 sec | Zero | Payments |
| **OBL** | **Deterministic** | **< 5 sec** | **Zero after checkpoint** | **Settlement** |
| **TLN/DRM** | **Probabilistic** | **~60 min** | **Exponentially decreasing** | **Monetary/Contracts** |

## Security Considerations

### Attack Vectors

#### 1. Checkpoint Withholding

**Attack:** Miner withholds checkpoint to delay finality

**Mitigation:**
- Checkpoints are consensus-enforced at fixed intervals
- Blocks without valid checkpoints at required heights are invalid
- Network rejects chains missing checkpoints

#### 2. Conflicting Checkpoints

**Attack:** Produce competing chains with different checkpoints

**Mitigation:**
- Nodes track finalized checkpoints
- Conflicting checkpoints trigger chain rejection
- Heaviest valid chain with consistent checkpoints wins

#### 3. State Root Manipulation

**Attack:** Create checkpoint with incorrect state root

**Mitigation:**
- All nodes independently compute state root
- Invalid state root makes block invalid
- Consensus rejects mismatched state roots

#### 4. Reorganization Attack

**Attack:** Mine longer chain to reverse finalized transactions

**Mitigation:**
- Nodes MUST NOT reorganize past finalized checkpoint
- Finality boundaries are consensus-critical
- Even with 51% hashrate, cannot reverse finalized state

### Failure Modes

#### Checkpoint Delay

**Scenario:** Network congestion delays checkpoint creation

**Impact:** Finality time increases but safety maintained

**Recovery:** Next checkpoint finalizes accumulated blocks

#### State Divergence

**Scenario:** Bug causes nodes to compute different state roots

**Impact:** Chain split at checkpoint

**Recovery:** Nodes on invalid fork reject checkpoint, sync to valid chain

#### Network Partition

**Scenario:** Network split prevents checkpoint propagation

**Impact:** Partitions cannot finalize independently

**Recovery:** Upon reconnection, partition with valid checkpoints prevails

## Light Client Support

Light clients can verify finality without full state:

```
LightClientFinality {
    // Minimum required data
    checkpoint_chain: []FinalityCheckpoint
    latest_checkpoint: FinalityCheckpoint
    
    // Verification
    VerifyTransaction(proof: FinalityProof) -> bool {
        // Verify proof against known checkpoints
        if !HasCheckpoint(proof.checkpoint):
            return false
        return VerifyFinalityProof(proof)
    }
    
    // Sync only checkpoints (not full state)
    SyncCheckpoints(from_height: uint64) {
        for height in range(from_height, latest_height, INTERVAL):
            checkpoint = FetchCheckpoint(height)
            ValidateAndStore(checkpoint)
    }
}
```

**Benefits:**
- Light clients can verify finality with minimal data
- Only need checkpoint chain, not full blocks
- Fast sync for institutional integrations

## Institutional Compliance

Finality mechanism supports regulatory requirements:

### Audit Trail

- Every finalized transaction has immutable receipt
- Settlement receipts contain all required metadata
- Cryptographic proof of finality timestamp
- Deterministic transaction ordering

### Reconciliation

- Reference IDs link to external systems
- Settlement receipts enable automatic reconciliation
- State proofs verify balance changes
- Idempotent settlement prevents double-counting

### Dispute Resolution

- Finality proofs are court-admissible (cryptographic evidence)
- Immutable transaction ordering
- Timestamp proof from consensus
- Complete audit trail

## Future Enhancements

Potential improvements to finality mechanism:

1. **Faster Checkpoints:** Reduce interval to 1-2 blocks
2. **Optimistic Finality:** Immediate finality with fraud proof fallback
3. **BLS Signatures:** Aggregate checkpoint signatures for efficiency
4. **Zero-Knowledge Proofs:** Privacy-preserving finality proofs
5. **Cross-Chain Finality:** Finality proofs for interoperability

## Implementation Notes

### Checkpoint Storage

Checkpoints are stored separately from blocks:

```
CheckpointDB {
    height -> FinalityCheckpoint
    hash -> FinalityCheckpoint
    latest_finalized -> height
}
```

### Mempool Policy

Mempool respects finality:

- Cannot include transactions conflicting with finalized state
- Nonce must be higher than finalized nonce
- Replace-by-fee disabled for finalized transactions

### RPC Interface

```
// Query finality status
obl_getFinalityStatus(txid) -> {
    status: "pending" | "confirmed" | "finalized",
    block_height: uint64,
    confirmations: uint32,
    checkpoint_height: uint64,
    finalized_at: timestamp
}

// Get finality proof
obl_getFinalityProof(txid) -> FinalityProof

// Get settlement receipt
obl_getSettlementReceipt(txid) -> SettlementReceipt

// Wait for finality (blocking)
obl_waitForFinality(txid, timeout) -> SettlementReceipt
```

## Testing Requirements

Comprehensive tests required for finality mechanism:

1. **Checkpoint Creation Tests**
   - Valid checkpoint at correct interval
   - Reject checkpoint at wrong height
   - Verify state root computation

2. **Reorganization Tests**
   - Cannot reorg past checkpoint
   - Competing chains with different checkpoints
   - Recovery from temporary fork

3. **Finality Proof Tests**
   - Generate and verify proofs
   - Invalid proof rejection
   - Light client verification

4. **Performance Tests**
   - Finality latency under load
   - Checkpoint propagation time
   - Proof generation/verification speed

5. **Attack Tests**
   - Checkpoint withholding
   - State root manipulation
   - Reorganization attempts
   - Network partition recovery

See [`tests/obolos/finality_tests.cpp`](../tests/obolos/) for implementation.

## References

- [Obolos Specification](../obolos.md) — Full OBL specification
- [Architecture](architecture.md) — System architecture
- [Fee Model](fees.md) — Fee predictability
- [Asset Model](../technical-specs/asset-model.md) — Multi-asset overview
