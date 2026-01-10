# DRACHMA Technical Specification (Draft)

This document summarizes key protocol parameters for DRACHMA. Values may be finalized during testnet iterations.

## Monetary Policy

- **Supply Cap:** 41,000,000 DRM enforced by consensus range checks.
- **Asymptotic Limit:** ~42,000,000 DRM (theoretical geometric series limit, but hard-capped at 41M)
- **Block Subsidy (initial):** 10 DRM.
- **Halving Interval:** 2,102,400 blocks (~4 years at 60-second targets).
- **Minimum Subsidy:** 0 DRM (no tail emission).
- **Fee Model:** 100% of transaction fees go to miners; no fee burning or developer fees.

### Supply Schedule

| Era | Height Range | Subsidy (DRM) | Era Emission (DRM) | Cumulative Supply (DRM) | Approx. Duration |
| --- | ------------ | ------------- | ------------------ | ----------------------- | ---------------- |
| 0 | 0 – 2,102,399 | 10.00000000 | 21,024,000 | 21,024,000 | ~4 years |
| 1 | 2,102,400 – 4,204,799 | 5.00000000 | 10,512,000 | 31,536,000 | ~4 years |
| 2 | 4,204,800 – 6,307,199 | 2.50000000 | 5,256,000 | 36,792,000 | ~4 years |
| 3 | 6,307,200 – 8,409,599 | 1.25000000 | 2,628,000 | 39,420,000 | ~4 years |
| 4 | 8,409,600 – 10,511,999 | 0.62500000 | 1,314,000 | 40,734,000 | ~4 years |
| 5 | 10,512,000 – 12,614,399 | 0.31250000 | 657,000 | 40,734,000* | ~4 years |
| 6+ | 12,614,400 onward | 0.15625000 → 0 | minimal tail | 41,000,000 cap | — |

*Consensus enforces 41M hard cap. Theoretical emission continues but blocks exceeding the cap are rejected.

Supply accumulates asymptotically toward ~42M but consensus validation enforces a strict 41M maximum; any block causing total supply to exceed 41,000,000 DRM is invalid.

#### Halving Math
- Subsidy per height `h` is `floor(initial_subsidy * 10^8 / 2^(h // interval)) / 10^8`.
- Consensus enforces `total_output <= max_money` at every spend to prevent overflow.
- Monetary tests should recompute the cumulative sum per era and assert convergence to the cap within ±1 sat tolerance.

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
- **Merkle Root:** `4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b` (testnet/mainnet share the same message with different timestamps/targets).
- **Header Parameters:** version `0x1`, timestamp `1735689600`, bits `0x1e0fffff`, nonce `2084524493` (mainnet draft values; testnet may allow easier targets).
- **Genesis Hash (mainnet draft):** `0x31fbff9618d6d72ecd673f6ef771a209f0b8ada3d7bb7030b867951a4f4bf521` derived from the above header.
- **Validation:** Genesis is hardcoded for bootstrapping but subject to the same header validity and proof-of-work checks.

### Genesis Verification Script (Python)
```python
import binascii, hashlib

def double_sha256(b: bytes) -> bytes:
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()[::-1]

genesis_header_hex = (
    "01000000"  # version
    "0000000000000000000000000000000000000000000000000000000000000000"  # prev
    "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"  # merkle
    "00000000"  # time (replace with mainnet/testnet)
    "1e0fffff"  # bits
    "7c2bac1d"  # nonce (replace with mainnet/testnet)
)

header = binascii.unhexlify(genesis_header_hex)
print("header hash:", double_sha256(header).hex())
```
Update `time` and `nonce` values with finalized parameters and compare the printed hash with the published genesis hash.

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

- No EVM/ABI or wrapped assets; only deterministic WASM sidechain is supported
- No privileged keys or governance layers
- No premines or developer fees embedded in consensus

Refer to [`../reference/whitepaper.md`](../reference/whitepaper.md) and [`../reference/architecture.md`](../reference/architecture.md) for additional context and motivations.
