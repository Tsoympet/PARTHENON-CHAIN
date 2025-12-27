# DRACHMA Technical Specification (Draft)

This document summarizes key protocol parameters for DRACHMA. Values may be finalized during testnet iterations.

## Monetary Policy

- **Supply Cap:** 42,000,000 DRM enforced by consensus range checks.
- **Block Subsidy (initial):** 50 DRM.
- **Halving Interval:** 2,102,400 blocks (~4 years at 60-second targets).
- **Minimum Subsidy:** 0 DRM (no tail emission).
- **Fee Model:** 100% of transaction fees go to miners; no fee burning or developer fees.

### Supply Schedule

| Era | Height Range | Subsidy (DRM) | Approx. Duration |
| --- | ------------ | ------------- | ---------------- |
| 0   | 0 – 2,102,399 | 50 | ~4 years |
| 1   | 2,102,400 – 4,204,799 | 25 | ~4 years |
| 2   | 4,204,800 – 6,307,199 | 12.5 | ~4 years |
| 3   | 6,307,200 – 8,409,599 | 6.25 | ~4 years |
| ... | ... | halving each era | ... |

Supply accumulates asymptotically to the 42M cap; the final subsidy era naturally rounds down to zero.

## Consensus Parameters

- **Proof of Work:** SHA-256d
- **Target Block Time:** 60 seconds
- **Difficulty Adjustment:** Retarget every 60 blocks (1-hour window) with clamped adjustments to prevent oscillations and timestamp abuse.
- **Block Size/Weight:** Conservative defaults; oversized blocks are rejected at consensus and policy layers.
- **Timestamp Rules:** Median-time-past enforcement and bounded future drift consistent with Bitcoin-derived safety limits.

## Transaction Model

- **UTXO-based ledger** with deterministic script-less validation
- **Signatures:** Schnorr over secp256k1
- **Hashing:** Tagged SHA-256 for transaction IDs; SHA-256d for Merkle roots
- **Standardness Policies:** Defined at Layer 2 (mempool) and must not modify consensus validity rules

## Network Parameters

- **Default Ports:**
  - P2P: 9333 (mainnet) / 19333 (testnet)
  - RPC: 8332 (mainnet) / 18332 (testnet)
  - Stratum/Work relay: 9333 (proxied by services; authenticate for pools)
- **Magic Bytes:** Distinct per network; nodes refuse messages with mismatched magic.
- **Message Integrity:** Checksummed payloads; DoS-resistant bounds on inventories, peer slots, and mempool admission.
- **Address Format:** Bech32m-style with human-readable prefix per network; checksum protects against typos.

## Genesis Block

- **Launch Statement:** Human-readable commitment encoded in the genesis coinbase, unspendable by consensus.
- **Merkle Root:** Computed over the single coinbase transaction using duplicate-last-leaf rule.
- **Timestamp & Difficulty:** Chosen to meet the SHA-256d target encoded in `nBits`; no premine or checkpointing is applied.
- **Validation:** Genesis is hardcoded for bootstrapping but subject to the same header validity and proof-of-work checks.

## Storage

- **Block Files:** Append-only with periodic compaction tools
- **Chainstate:** Key-value store (LevelDB/SQLite variant) with snapshotting support under evaluation
- **Indexes:** Optional transaction/address indexes provided by Layer 2 services only

## Upgrade Policy

- Consensus modifications require:
  - Public proposal with rationale and testing evidence
  - Testnet activation with monitoring
  - Clear activation mechanism (flag day or version bits) and rollback plan

## Non-Goals

- No on-chain smart contracts or programmable VM
- No privileged keys or governance layers
- No premines or developer fees embedded in consensus

Refer to [`docs/whitepaper.md`](whitepaper.md) and [`docs/architecture.md`](architecture.md) for additional context and motivations.
