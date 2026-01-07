# Fee Logic & Predictability

## Overview

PARTHENON CHAIN implements different fee models for each asset, optimized for their respective use cases. This document specifies the fee calculation logic, predictability guarantees, and anti-spam protection mechanisms.

## Fee Models by Asset

### TLN (Talanton) — Market-Based Fees

**Model:** Dynamic fee market similar to Bitcoin

**Characteristics:**
- Fee rate determined by mempool demand
- Users bid for block space
- Variable fees based on transaction size and priority
- No guaranteed fee predictability

**Use Case:** Best for monetary transactions where occasional fee volatility is acceptable

### DRM (Drachma) — Market-Based Fees

**Model:** Dynamic fee market for smart contract execution

**Characteristics:**
- Gas-like metering for contract execution
- Fee proportional to computational cost
- Market-driven gas price
- Variable fees based on contract complexity

**Use Case:** Smart contract deployment and execution

### OBL (Obolos) — Flat & Predictable Fees

**Model:** Fixed base fee + optional bounded priority fee

**Characteristics:**
- **Deterministic base fee:** Known in advance
- **Bounded priority fee:** Optional, capped maximum
- **No fee market volatility:** Stable costs for budgeting
- **Anti-spam protection:** Minimum fee enforced

**Use Case:** Institutional settlement requiring cost predictability

## Obolos Fee Structure

### Base Fee

The base fee is a fixed amount required for all transactions.

```
BASE_FEE = 1000 satoshis  // 0.00001 OBL
```

**Properties:**
- **Fixed Amount:** Does not change based on demand
- **Consensus-Enforced:** Transactions with insufficient base fee are invalid
- **Predictable:** Known months/years in advance
- **Anti-Spam:** High enough to prevent dust attacks

**Adjustment Mechanism:**
- Base fee can be adjusted via consensus upgrade (rare)
- Changes require network-wide coordination
- Advance notice (6+ months) for institutional planning

### Priority Fee

Optional additional fee for faster inclusion.

```
PRIORITY_FEE = user_specified
MAX_PRIORITY_FEE = 100000 satoshis  // 0.001 OBL
```

**Properties:**
- **Optional:** Users can set to 0
- **Bounded:** Cannot exceed maximum
- **Sorting:** Higher priority fees get earlier inclusion
- **Non-Consensus:** Mempool policy, not consensus rule

**Benefits:**
- Users can accelerate urgent transactions
- Bounded maximum prevents fee market runaway
- Predictable worst-case cost (base + max priority)

### Total Fee Calculation

```
total_fee = BASE_FEE + min(priority_fee, MAX_PRIORITY_FEE)

Example 1: Standard transaction
  base_fee = 1000 satoshis
  priority_fee = 0
  total_fee = 1000 satoshis

Example 2: High-priority transaction
  base_fee = 1000 satoshis
  priority_fee = 50000 satoshis
  total_fee = 51000 satoshis

Example 3: Capped priority
  base_fee = 1000 satoshis
  priority_fee = 200000 satoshis (exceeds max)
  total_fee = 101000 satoshis (base + max priority)
```

## Fee Burning

Obolos supports optional deterministic fee burning.

```
FEE_BURNING_ENABLED = true  // Configurable via consensus
BURN_PERCENTAGE = 100%      // Percentage of fees to burn
```

**With Fee Burning:**
```
ProcessFee(transaction):
    total_fee = transaction.fee
    burned_amount = total_fee * BURN_PERCENTAGE / 100
    miner_reward = total_fee - burned_amount
    
    Burn(burned_amount)
    AddToBlock(miner_reward)
```

**Benefits:**
- Deflationary pressure on supply
- Aligns user and holder incentives
- Predictable supply curve

**Risks:**
- Miner revenue reduction
- Must ensure block subsidies remain sufficient

**Current Setting:** 100% burn (all fees burned, miners earn only block subsidy)

## Anti-Spam Protection

Multiple mechanisms prevent network abuse:

### 1. Minimum Fee Requirement

**Rule:** All transactions must pay at least `BASE_FEE`

**Enforcement:**
```
ValidateTransaction(tx):
    if tx.fee < BASE_FEE:
        return Error("Fee below minimum")
    // ... other validation
```

**Effect:** Makes spam attacks expensive

### 2. Nonce-Based Rate Limiting

**Rule:** One transaction per nonce, sequential ordering

**Enforcement:**
```
ValidateTransaction(tx):
    if tx.nonce != accounts[tx.sender].nonce + 1:
        return Error("Invalid nonce")
```

**Effect:** Prevents transaction flooding from single account

### 3. Mempool Admission Policy

**Rule:** Mempool prioritizes by fee, bounded size

```
MEMPOOL_MAX_SIZE = 10000 transactions
MEMPOOL_MIN_FEE = BASE_FEE

AdmitToMempool(tx):
    if tx.fee < MEMPOOL_MIN_FEE:
        return Reject("Fee too low")
    
    if mempool.size >= MEMPOOL_MAX_SIZE:
        if tx.fee <= mempool.min_fee():
            return Reject("Mempool full, fee too low")
        mempool.evict_lowest_fee()
    
    mempool.add(tx)
```

**Effect:** Economic limit on mempool spam

### 4. Account Minimum Balance

**Rule:** Accounts must maintain minimum balance for activity

```
MIN_ACCOUNT_BALANCE = 10000 satoshis  // 0.0001 OBL

ValidateTransaction(tx):
    if accounts[tx.sender].balance < tx.amount + tx.fee + MIN_ACCOUNT_BALANCE:
        return Error("Insufficient balance")
```

**Effect:** Prevents dust accounts from spamming

## Fee Predictability Guarantees

### Guarantee 1: Bounded Worst-Case

**Statement:** Maximum possible fee is known in advance

```
MAX_FEE = BASE_FEE + MAX_PRIORITY_FEE
        = 1000 + 100000
        = 101000 satoshis
        = 0.00101 OBL
```

**Benefit:** Institutional budgets can allocate exact maximum cost

### Guarantee 2: Deterministic Base

**Statement:** Base fee changes require consensus upgrade

**Implication:**
- Months of advance notice
- Predictable 6-12 month planning horizon
- No surprise fee increases

### Guarantee 3: Linear Scaling

**Statement:** Fee does not depend on network congestion

**Contrast with Market-Based:**
- Market fees: 10x-100x variation during congestion
- Flat fees: No variation regardless of demand

**Trade-off:** May require rate limiting during extreme demand

### Guarantee 4: Transaction Size Independence

**Statement:** Fee is per-transaction, not per-byte

**Benefit:**
- Simple cost calculation
- No need to estimate transaction size
- Predictable for all transaction types

**Trade-off:** Large transactions pay same as small (bounded by max size)

## Fee Estimation

### For Users

```
EstimateFee(priority_level):
    base = BASE_FEE
    
    if priority_level == "standard":
        priority = 0
    elif priority_level == "fast":
        priority = MAX_PRIORITY_FEE / 2
    elif priority_level == "instant":
        priority = MAX_PRIORITY_FEE
    
    return base + priority

Example:
  standard: 1000 satoshis (0.00001 OBL)
  fast: 51000 satoshis (0.00051 OBL)
  instant: 101000 satoshis (0.00101 OBL)
```

### For Institutional Planners

```
AnnualFeeBudget(transaction_count, priority_mix):
    standard_count = transaction_count * priority_mix["standard"]
    fast_count = transaction_count * priority_mix["fast"]
    instant_count = transaction_count * priority_mix["instant"]
    
    total_fees = (
        standard_count * EstimateFee("standard") +
        fast_count * EstimateFee("fast") +
        instant_count * EstimateFee("instant")
    )
    
    return total_fees

Example:
  Annual transactions: 1,000,000
  Mix: 70% standard, 20% fast, 10% instant
  
  Budget = 700,000 * 1000 + 200,000 * 51000 + 100,000 * 101000
         = 700,000,000 + 10,200,000,000 + 10,100,000,000
         = 21,000,000,000 satoshis
         = 210 OBL per year
```

## Fee Market Dynamics

### Mempool Sorting

```
MempoolOrdering:
    1. Primary: fee (highest first)
    2. Secondary: nonce (lowest first for same sender)
    3. Tertiary: timestamp (oldest first)

GetNextTransactions(block_size_limit):
    candidates = mempool.sorted_by_fee()
    selected = []
    
    for tx in candidates:
        if total_size + tx.size > block_size_limit:
            break
        if ValidateNonceSequence(tx, selected):
            selected.append(tx)
    
    return selected
```

**Properties:**
- Deterministic ordering
- Higher fees get priority
- Nonce sequence maintained per account

### No Fee Escalation

Unlike dynamic fee markets, OBL fees don't escalate during congestion:

**Market-Based (TLN/DRM):**
```
Normal: 0.001 fee
Congestion: 0.1 fee (100x increase)
Extreme: 1.0 fee (1000x increase)
```

**Flat Fee (OBL):**
```
Normal: 0.00001 base + optional priority
Congestion: 0.00001 base + optional priority (same)
Extreme: 0.00001 base + optional priority (same)
```

**Trade-off:**
- Benefit: Predictable costs
- Risk: May require rate limiting during attacks

## Rate Limiting (Anti-DoS)

When demand exceeds capacity, rate limiting applies:

### Per-Account Limit

```
MAX_TX_PER_ACCOUNT_PER_BLOCK = 10

BuildBlock():
    account_tx_count = {}
    
    for tx in mempool.sorted_by_fee():
        if account_tx_count[tx.sender] >= MAX_TX_PER_ACCOUNT_PER_BLOCK:
            continue  // Skip, account limit reached
        
        if CanInclude(tx):
            block.add(tx)
            account_tx_count[tx.sender] += 1
```

**Effect:** Prevents single account from dominating block

### Global Throughput Limit

```
MAX_TRANSACTIONS_PER_BLOCK = 1000

BuildBlock():
    tx_count = 0
    
    for tx in mempool.sorted_by_fee():
        if tx_count >= MAX_TRANSACTIONS_PER_BLOCK:
            break
        
        if CanInclude(tx):
            block.add(tx)
            tx_count += 1
```

**Effect:** Bounded computational load per block

## Fee Revenue Model

### Miner Revenue

Without fee burning:
```
miner_revenue = block_subsidy + sum(transaction_fees)
```

With 100% fee burning (current):
```
miner_revenue = block_subsidy
burned = sum(transaction_fees)
```

**Implications:**
- Miners rely on block subsidy, not fees
- Requires sufficient subsidy to ensure security
- Deflationary effect from burned fees

### Security Budget

```
Annual Security Budget:
  Block subsidy per block: 8 OBL
  Blocks per year: 525,600 (60s blocks)
  Annual issuance: 4,204,800 OBL
  
  At $1/OBL: $4.2M annual security budget
  At $10/OBL: $42M annual security budget
```

Must ensure security budget remains adequate as subsidy halves.

## Comparison with Other Models

| Model | Predictability | Anti-Spam | Fairness | Efficiency |
|-------|---------------|-----------|----------|------------|
| Bitcoin (market) | Low | High | Medium | High |
| Ethereum (gas) | Low | High | Medium | Medium |
| EIP-1559 (base+tip) | Medium | High | High | High |
| **OBL (flat+priority)** | **Very High** | **High** | **High** | **High** |

## Institutional Benefits

### Budget Planning

- Fixed base fee enables precise annual budgeting
- Bounded priority fee sets maximum cost
- No surprise fee spikes
- Multi-year planning possible

### Cost Predictability

- Settlement cost known before transaction
- No need for fee estimation oracles
- Eliminates fee estimation errors
- Reduces transaction failures due to insufficient fees

### Compliance

- Deterministic fee calculation
- Auditable fee payments
- Immutable fee records in settlement receipts
- Clear cost attribution

### Reconciliation

- Fixed fees simplify accounting
- Predictable line items
- No variable fee adjustments
- Automated reconciliation

## Future Considerations

### Fee Adjustment Mechanism

If base fee needs adjustment:

1. **Governance-Free Proposal:**
   - Code change in new release
   - Deployment via version bits activation
   - 6+ month activation delay

2. **Signaling:**
   - Miners signal readiness in block version
   - Activates when threshold reached
   - Grace period before enforcement

3. **Implementation:**
   - New `BASE_FEE_V2` value
   - Activation height specified
   - Legacy transactions rejected after height

### Dynamic Priority Cap

Potential future enhancement:

```
MAX_PRIORITY_FEE = f(block_utilization)

If block_utilization < 50%:
    MAX_PRIORITY_FEE = 100000
Else if block_utilization < 80%:
    MAX_PRIORITY_FEE = 200000
Else:
    MAX_PRIORITY_FEE = 500000
```

**Benefit:** More responsive to extreme demand while maintaining bounds

**Risk:** Reduces predictability

## Testing Requirements

Comprehensive fee tests required:

1. **Base Fee Enforcement**
   - Reject transactions with fee < BASE_FEE
   - Accept transactions with fee >= BASE_FEE

2. **Priority Fee Capping**
   - Priority fee capped at MAX_PRIORITY_FEE
   - Total fee = base + min(priority, max)

3. **Fee Burning**
   - Verify fees are burned (not paid to miner)
   - Check total supply decreases correctly

4. **Mempool Ordering**
   - Higher fees sorted first
   - Nonce sequence maintained

5. **Rate Limiting**
   - Per-account limit enforced
   - Global throughput limit enforced

6. **Fee Predictability**
   - Estimate fee accurately for all scenarios
   - Worst-case fee matches expectation

7. **Stress Tests**
   - High-volume transaction load
   - Mempool saturation
   - Fee-based prioritization under load

See [`tests/obolos/fee_tests.cpp`](../tests/obolos/) for implementation.

## References

- [Obolos Specification](../obolos.md) — Full OBL specification
- [Finality Guarantees](finality.md) — Finality mechanism
- [Architecture](architecture.md) — System architecture
- [Asset Model](../technical-specs/asset-model.md) — Multi-asset overview
