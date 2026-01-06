# Obolos (OBL) — Settlement Token Specification

## Overview

Obolos (OBL) is a settlement-focused cryptocurrency designed for institutional-grade payment processing with deterministic finality, low predictable fees, and account-based state management. Unlike Talanton (TLN) and Drachma (DRM), which follow a UTXO model, Obolos uses an account-based ledger optimized for high-throughput payment settlement.

## Asset Properties

- **Asset ID:** 2
- **Symbol:** OBL
- **Consensus Model:** Proof-of-Work only
- **Supply Cap:** 61,000,000 OBL
- **Initial Block Subsidy:** 8 OBL
- **Halving Interval:** 2,102,400 blocks (~4 years at 60s target)
- **Ledger Model:** Account-based (not UTXO)
- **Primary Use Case:** Payment settlement, institutional transfers, routing/path-payment

## Core Features

### 1. Fast Finality (Settlement Finality)

Obolos transactions achieve deterministic finality within sub-5 second targets through a finality mechanism separate from block confirmation.

**Characteristics:**
- **Target Finality Time:** < 5 seconds
- **Deterministic Confirmation:** Once finalized, transactions are irreversible
- **No Probabilistic Reorgs:** Finalized transactions cannot be reorganized
- **Finality Proofs:** Cryptographic proof of finality available for each transaction

**Implementation:**
- Finality checkpoints recorded every N blocks
- Finality proof includes block hash, transaction merkle proof, and checkpoint signature
- Transactions reference latest finality checkpoint
- Nodes enforce finality boundaries during chain reorganization

### 2. Account-Based Ledger

Unlike TLN and DRM which use UTXO, Obolos maintains account state with balances and nonces.

**Account Structure:**
```
Account {
    address:    [20 bytes]  // Human-readable encoding (bech32-style)
    balance:    uint64      // Current balance in smallest unit
    nonce:      uint64      // Transaction counter for replay protection
    metadata:   bytes       // Optional institutional metadata
}
```

**Benefits:**
- Human-readable addresses (no complex script evaluation)
- Direct balance queries without UTXO scanning
- Native support for atomic updates
- Simplified multi-party settlement

### 3. Nonce-Based Replay Protection

Each account maintains a monotonically increasing nonce that must match the next expected transaction sequence number.

**Rules:**
- Transaction nonce must equal `account.nonce + 1`
- Failed transactions still increment nonce to prevent retry attacks
- Nonce gaps are rejected
- Out-of-order transactions are invalid

**Benefits:**
- Prevents transaction replay across chains or after reorgs
- Enables deterministic transaction ordering
- Simplifies mempool management

### 4. Native Payment Channel Support

Obolos includes protocol-level primitives for payment channels and conditional settlement.

**Features:**
- **Escrow Primitives:** Time-locked balance escrow
- **Conditional Release:** Hash-locked and signature-locked releases
- **Atomic Settlement Hooks:** Multi-party atomic settlement
- **Channel State Updates:** Off-chain state with on-chain settlement

**Use Cases:**
- Micropayment channels
- Cross-border settlement corridors
- Conditional payment release (delivery vs payment)
- Programmatic settlement logic

### 5. Low & Predictable Fees

Obolos uses a flat or near-flat fee model to ensure cost predictability for institutional users.

**Fee Model:**
- **Base Fee:** Fixed fee per transaction (configurable)
- **Priority Fee:** Optional small increment for faster inclusion
- **Fee Burning:** Optional deterministic fee burning (configurable)
- **Anti-Spam Protection:** Minimum fee threshold enforced

**Fee Structure:**
```
total_fee = base_fee + priority_fee
base_fee = FIXED_BASE_FEE  // e.g., 0.001 OBL
priority_fee = user_specified  // optional, bounded
```

**Benefits:**
- Predictable cost for settlement operations
- No gas price volatility
- Transparent fee calculation
- Institutional budgeting compatibility

### 6. Path-Payment / Routing Hooks

Obolos supports multi-hop settlement routing at the protocol level.

**Features:**
- **Native Routing Logic:** Protocol-aware path discovery
- **Multi-Hop Settlement:** Automatic routing through intermediate accounts
- **No AMM Required:** Direct routing without liquidity pools
- **Atomic Path Execution:** Entire route succeeds or fails atomically

**Path Settlement Transaction:**
```
PathPayment {
    sender:         address
    receiver:       address
    amount:         uint64
    max_hops:       uint8
    route_hint:     []address  // optional routing suggestion
    deadline:       timestamp
}
```

### 7. Institutional Settlement Readiness

Obolos includes protocol-level features for institutional compliance and audit requirements.

**Transaction Metadata Fields:**
- **Reference ID:** External reference identifier (bounded, 32 bytes)
- **Descriptor:** Human-readable transaction description (bounded, 128 bytes)
- **Memo Field:** Optional memo (bounded, 256 bytes, non-executable)
- **Timestamp:** Ledger-recorded timestamp (block time)
- **Settlement Receipt:** Cryptographic receipt with finality proof

**Settlement Receipt Structure:**
```
SettlementReceipt {
    transaction_id:     bytes32
    block_height:       uint64
    block_hash:         bytes32
    finality_proof:     bytes
    timestamp:          uint64
    reference_id:       bytes32
    status:             enum { Pending, Finalized, Failed }
}
```

**Protocol Features:**
- Deterministic transaction ordering (by nonce)
- Idempotent transaction handling (duplicate nonce rejection)
- Settlement receipts with finality proofs
- Time-stamped ledger entries
- Bounded metadata fields (no arbitrary execution)

**Explicit Exclusions:**
The protocol does NOT include:
- KYC (Know Your Customer) logic
- AML (Anti-Money Laundering) enforcement
- Identity verification or management
- Compliance rule enforcement

These responsibilities belong to ecosystem layers (wallets, exchanges, custodians), not the base protocol.

## Transaction Structure

```
ObolosTransaction {
    version:        uint32      // Transaction version
    sender:         address     // Sender account
    receiver:       address     // Receiver account
    amount:         uint64      // Amount to transfer
    nonce:          uint64      // Sender's current nonce + 1
    fee:            uint64      // Total fee
    reference_id:   bytes32     // Optional institutional reference
    memo:           bytes       // Optional memo (max 256 bytes)
    timestamp:      uint64      // Transaction creation time
    signature:      bytes       // Schnorr signature
}
```

## Validation Rules

1. **Nonce Check:** `tx.nonce == account[sender].nonce + 1`
2. **Balance Check:** `account[sender].balance >= tx.amount + tx.fee`
3. **Fee Check:** `tx.fee >= MIN_BASE_FEE`
4. **Signature Verification:** Valid Schnorr signature over transaction digest
5. **Memo Bound:** `len(tx.memo) <= MAX_MEMO_SIZE`
6. **Reference ID Bound:** `len(tx.reference_id) <= 32`
7. **No Double-Spend:** Nonce prevents replay
8. **Finality Respect:** Cannot modify finalized state

## State Transitions

```
ProcessTransaction(tx):
    // 1. Validation
    if tx.nonce != accounts[tx.sender].nonce + 1:
        return Error("Invalid nonce")
    
    if accounts[tx.sender].balance < tx.amount + tx.fee:
        return Error("Insufficient balance")
    
    if !VerifySignature(tx):
        return Error("Invalid signature")
    
    // 2. State Update (atomic)
    accounts[tx.sender].balance -= (tx.amount + tx.fee)
    accounts[tx.sender].nonce += 1
    accounts[tx.receiver].balance += tx.amount
    
    // 3. Fee Handling
    if FEE_BURNING_ENABLED:
        burn(tx.fee)
    else:
        miner_reward += tx.fee
    
    // 4. Finality Update
    if ShouldUpdateFinality():
        CreateFinalityCheckpoint()
    
    return Success
```

## Finality Mechanism

### Finality Checkpoint

Every `FINALITY_CHECKPOINT_INTERVAL` blocks, a finality checkpoint is created.

```
FinalityCheckpoint {
    block_height:       uint64
    block_hash:         bytes32
    state_root:         bytes32      // Merkle root of all account states
    checkpoint_hash:    bytes32      // Hash of checkpoint data
    validator_set:      []address    // PoW miners (not separate validators)
}
```

### Finality Rules

1. Transactions in blocks before the latest finality checkpoint are **finalized**
2. Chain reorganizations cannot affect finalized blocks
3. Nodes reject reorganizations that conflict with finalized checkpoints
4. Finality proofs are verifiable by light clients

### Finality Proof

```
FinalityProof {
    transaction:        ObolosTransaction
    block_height:       uint64
    block_hash:         bytes32
    merkle_proof:       []bytes32
    checkpoint:         FinalityCheckpoint
    checkpoint_proof:   bytes
}
```

## Network Constants

```
// Timing
FINALITY_CHECKPOINT_INTERVAL = 5 blocks  // ~5 minutes at 60s blocks
TARGET_FINALITY_TIME = 5 seconds         // Target for checkpoint creation

// Fees
MIN_BASE_FEE = 1000 satoshis            // 0.00001 OBL
MAX_PRIORITY_FEE = 100000 satoshis      // 0.001 OBL
FEE_BURNING_ENABLED = true              // Optional, configurable

// Bounds
MAX_MEMO_SIZE = 256 bytes
MAX_REFERENCE_ID_SIZE = 32 bytes
MAX_DESCRIPTOR_SIZE = 128 bytes
MAX_PATH_HOPS = 5

// Replay Protection
NONCE_START = 0
NONCE_GAP_TOLERANCE = 0                 // No gaps allowed
```

## RPC Interface

### Query Operations

```
obl_getBalance(address) -> uint64
obl_getNonce(address) -> uint64
obl_getAccount(address) -> Account
obl_getTransaction(txid) -> ObolosTransaction
obl_getFinalityStatus(txid) -> { finalized: bool, checkpoint: FinalityCheckpoint }
obl_getFinalityProof(txid) -> FinalityProof
```

### Transaction Operations

```
obl_sendTransaction(tx) -> txid
obl_sendPathPayment(path_payment) -> txid
obl_getSettlementReceipt(txid) -> SettlementReceipt
```

### Channel Operations

```
obl_openChannel(escrow_tx) -> channel_id
obl_updateChannel(state_update) -> success
obl_closeChannel(channel_id, final_state) -> txid
```

## Security Considerations

### Replay Protection

- Nonce mechanism prevents transaction replay
- Failed transactions increment nonce to prevent retry storms
- Cross-chain replay prevented by chain-specific address encoding

### Finality Attacks

- Finality checkpoints are consensus-critical
- Cannot finalize conflicting states
- Nodes enforce finality boundaries strictly

### Fee Manipulation

- Base fee prevents dust/spam attacks
- Priority fee bounded to prevent fee market manipulation
- Deterministic fee calculation prevents front-running

### Account Exhaustion

- Accounts are created on-demand (first receive)
- No artificial account limits
- State pruning for zero-balance accounts (configurable)

## Migration from UTXO (TLN/DRM)

Obolos uses a different ledger model than TLN and DRM. Cross-asset transfers require explicit conversion:

1. **UTXO → Account:** Lock TLN/DRM in conversion contract, mint equivalent OBL to account
2. **Account → UTXO:** Burn OBL from account, unlock equivalent TLN/DRM from conversion contract

Conversion is handled by Layer 2 bridge services, not Layer 1 consensus.

## Performance Targets

- **Transaction Throughput:** 1000+ tx/s sustained
- **Finality Time:** < 5 seconds
- **Account Query Time:** < 10ms
- **Path Payment Resolution:** < 100ms
- **State Sync Time:** < 1 minute for full account set

## Auditability

All Obolos operations are deterministic and auditable:

1. **Transaction Ordering:** Strictly by nonce
2. **State Transitions:** Fully deterministic given inputs
3. **Finality Proofs:** Cryptographically verifiable
4. **Settlement Receipts:** Immutable and tamper-proof
5. **Account History:** Complete transaction history per account

## Testing Requirements

See [`tests/obolos/`](../tests/obolos/) for comprehensive test coverage:

- Settlement speed tests (finality latency)
- Finality proof verification tests
- Replay protection tests (nonce validation)
- Fee predictability tests (base + priority)
- Stress tests for payment throughput (1000+ tx/s)
- Path payment routing tests
- Channel lifecycle tests
- Idempotent transaction handling

## Comparison with Other Assets

| Feature | TLN | DRM | OBL |
|---------|-----|-----|-----|
| Consensus | PoW | PoW | PoW |
| Ledger Model | UTXO | UTXO | Account-based |
| Primary Use | Monetary base | Smart contracts | Settlement |
| Finality | Probabilistic (~60 min) | Probabilistic (~60 min) | Deterministic (< 5s) |
| Fee Model | Market-based | Market-based | Flat + priority |
| Replay Protection | UTXO uniqueness | UTXO uniqueness | Nonce-based |
| Address Type | Scriptable | Scriptable | Account (bech32) |
| Payment Channels | Layer 2 | Layer 2 | Native (Layer 1) |
| Institutional Features | None | None | Full support |

## Future Enhancements

Potential future improvements (not currently implemented):

- Multi-signature accounts
- Threshold signatures for institutional custody
- Time-weighted average fees for long-term planning
- Optimistic finality (< 1 second) with fraud proofs
- Privacy-preserving payment paths (zk-proofs)

## References

- [Asset Model](technical-specs/asset-model.md) — Multi-asset overview
- [Protocol Specification](technical-specs/protocol.md) — General protocol rules
- [Architecture](reference/architecture.md) — System layer separation
- [Finality Specification](finality.md) — Detailed finality mechanism
- [Fee Model](fees.md) — Fee calculation and predictability
