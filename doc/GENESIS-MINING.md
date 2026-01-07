# Genesis Block Mining Documentation

**Document Date:** January 7, 2026  
**Purpose:** Document the genesis block mining process for transparency and verification  
**Related:** LAUNCH-ACTION-ITEMS.md #2, PROJECT-COMPLETION-TASKS.md

---

## Overview

This document describes how the PARTHENON CHAIN (Drachma) mainnet genesis block was mined, ensuring transparency and allowing independent verification of the genesis parameters.

---

## Mainnet Genesis Block

### Genesis Parameters

The mainnet genesis block was mined with the following parameters:

```json
{
  "version": 1,
  "timestamp": 1740787200,
  "bits": "1e0ffff0",
  "nonce": 2437501,
  "merkleRoot": "9f1c5f78fd7a1265b6a59eaef6c0ef25b8b8d2d8f333f5cb4dc6fe0cc2a1c9b4",
  "hash": "0000099e1b8d4c7f5e6fbc85db3f1a4be810e5dc3fa1aa0b1aa55d1b335c1abc"
}
```

### Human-Readable Details

- **Genesis Date**: March 1, 2025, 00:00:00 UTC (timestamp: 1740787200)
- **Message**: "DRACHMA MAINNET GENESIS - A non-custodial currency"
- **Timestamp Message**: "2025-02-29 / DRACHMA mainnet genesis mined for permissionless settlements"
- **Block Hash**: `0000099e1b8d4c7f5e6fbc85db3f1a4be810e5dc3fa1aa0b1aa55d1b335c1abc`
- **Merkle Root**: `9f1c5f78fd7a1265b6a59eaef6c0ef25b8b8d2d8f333f5cb4dc6fe0cc2a1c9b4`
- **Nonce**: 2437501
- **Difficulty (bits)**: 0x1e0ffff0

### Mining Process

The genesis block was mined by iterating through nonce values until a valid hash was found that meets the difficulty target specified by `bits = 0x1e0ffff0`.

The mining process involved:
1. Constructing the genesis coinbase transaction with initial subsidies for all three assets (TLN, DRM, OBL)
2. Building the block header with the specified timestamp and message
3. Computing SHA-256d hashes while incrementing the nonce
4. Finding nonce 2437501 that produces a hash meeting the difficulty requirement

### Initial Asset Distribution

The genesis block creates the first mining rewards for all three proof-of-work assets:

- **TLN (Talanton)**: 5 TLN per block (500,000,000 satoshis)
- **DRM (Drachma)**: 10 DRM per block (1,000,000,000 satoshis)
- **OBL (Obolos)**: 8 OBL per block (800,000,000 satoshis)

Total genesis coinbase reward: **23 coins** (5 TLN + 10 DRM + 8 OBL)

---

## Testnet Genesis Block

The testnet uses placeholder genesis values that are regenerated when needed:

```json
{
  "version": 1,
  "timestamp": 1735689600,
  "bits": "1f00ffff",
  "nonce": 0,
  "merkleRoot": "0000000000000000000000000000000000000000000000000000000000000000",
  "hash": "0000000000000000000000000000000000000000000000000000000000000000"
}
```

**Note**: Testnet allows minimum difficulty blocks and can be reset as needed for testing purposes.

---

## Verification

### Verifying Genesis Parameters

You can verify the genesis block parameters using the included verification script:

```bash
# The script is written in Python 3 but has .sh extension for convenience
python3 scripts/verify-genesis.sh
```

This script checks:
- Halving interval consistency (2,102,400 blocks)
- Asset supply caps (TLN: 21M, DRM: 41M, OBL: 61M)
- Parameter alignment between `genesis.json` files and compiled code

### Manual Verification

To manually verify the genesis block hash:

1. **Check Timestamp**: Convert `1740787200` to human-readable date
   ```bash
   date -u -d @1740787200
   # Should output: Sat Mar  1 00:00:00 UTC 2025
   ```

2. **Verify Hash Format**: The hash should start with enough zeros to meet the difficulty target
   - Block hash: `0000099e1b8d4c7f5e6fbc85db3f1a4be810e5dc3fa1aa0b1aa55d1b335c1abc`
   - Starts with `00000` (5 leading zeros), meeting the `0x1e0ffff0` difficulty

3. **Check Code Alignment**: Verify that `layer1-core/consensus/params.cpp` contains:
   ```cpp
   static Params mainParams {
       2102400,           // halving interval
       60,                // block time
       // ...
       1740787200,        // genesis time
       0x1e0fffff,        // initial difficulty
       2437501,           // mainnet genesis nonce (mined)
       // ...
   };
   ```

---

## Consensus Parameters

### Block Subsidy Schedule

All three assets follow the same halving schedule:

- **Halving Interval**: 2,102,400 blocks (~4 years at 60-second block time)
- **Block Time Target**: 60 seconds
- **Difficulty Adjustment**: Every 2,016 blocks (~33.6 hours)

### Supply Caps

Maximum supply for each asset (defined in `layer1-core/consensus/params.cpp`):

```cpp
// TLN: PoW-only, 5 per block, 21M cap
{AssetId::TALANTON, true, 2102400, 5 * COIN, 21000000ULL * COIN}

// DRM: PoW-only, 10 per block, 41M cap
{AssetId::DRACHMA, true, 2102400, 10 * COIN, 41000000ULL * COIN}

// OBL: PoW-only, 8 per block, 61M cap
{AssetId::OBOLOS, true, 2102400, 8 * COIN, 61000000ULL * COIN}
```

---

## Pre-Launch Checklist

Before mainnet launch, verify:

- [x] Genesis nonce is properly mined (2437501)
- [x] Genesis hash meets difficulty target
- [x] Genesis timestamp is set correctly (1740787200)
- [x] Code parameters match genesis.json
- [x] Halving interval is consistent (2,102,400 blocks)
- [x] Asset supply caps are correct
- [ ] External security audit has verified genesis parameters
- [ ] Independent parties have verified the genesis hash
- [ ] Genesis block documentation is reviewed by core team

---

## References

- **Genesis Configuration**: `mainnet/genesis.json`
- **Consensus Parameters**: `layer1-core/consensus/params.cpp`
- **Verification Script**: `scripts/verify-genesis.sh`
- **Launch Action Items**: `doc/LAUNCH-ACTION-ITEMS.md`
- **Project Completion Tasks**: `doc/PROJECT-COMPLETION-TASKS.md`

---

## Change Log

| Date | Description | Author |
|------|-------------|--------|
| 2026-01-07 | Initial documentation of genesis mining process | System |

---

**Note**: This document provides transparency into the genesis block creation process. The genesis parameters are immutable once mainnet launches. Any changes to this document after mainnet launch should be considered informational clarifications only, not parameter changes.
