# Wallet Recovery Testing Procedures

**Version:** 1.0  
**Last Updated:** January 6, 2026  
**Purpose:** Document procedures for testing wallet recovery scenarios

This document provides comprehensive test procedures to validate wallet recovery functionality before mainnet launch.

---

## Table of Contents

1. [Overview](#overview)
2. [Test Prerequisites](#test-prerequisites)
3. [Seed Phrase Recovery](#seed-phrase-recovery)
4. [Encrypted Wallet Recovery](#encrypted-wallet-recovery)
5. [Multi-Asset Wallet Recovery](#multi-asset-wallet-recovery)
6. [Edge Cases](#edge-cases)
7. [Automated Testing](#automated-testing)

---

## Overview

Wallet recovery is critical for user fund security. These tests validate that users can reliably recover their wallets using:
- 24-word mnemonic (seed phrase)
- Encrypted wallet.dat file
- Backup files

**Testing Philosophy:**
- Test on testnet first
- Use small amounts initially
- Document all steps and results
- Verify recovery across different platforms

---

## Test Prerequisites

### Environment Setup

1. **Clean test environment:**
   ```bash
   # Create isolated test directory
   mkdir -p ~/drachma-test-recovery
   cd ~/drachma-test-recovery
   ```

2. **Backup any existing wallet:**
   ```bash
   # If you have a real wallet, back it up first!
   cp ~/.drachma/wallet.dat ~/.drachma/wallet.dat.backup
   ```

3. **Use testnet:**
   ```bash
   # Always test on testnet first
   export DRACHMA_TESTNET=1
   ```

### Required Tools

- DRACHMA node and CLI (latest version)
- Text editor for recording results
- Checksum utility (sha256sum or similar)
- Stopwatch for timing tests

---

## Seed Phrase Recovery

### Test 1: Basic Seed Phrase Recovery

**Objective:** Verify a wallet can be recovered from a 24-word mnemonic.

**Procedure:**

1. **Create new wallet:**
   ```bash
   # Start with fresh data directory
   rm -rf ~/.drachma-test
   drachma-node --datadir=~/.drachma-test --testnet &
   sleep 5
   
   # Create wallet
   drachma-cli --datadir=~/.drachma-test --testnet createwallet "test1"
   ```

2. **Generate and record seed:**
   ```bash
   # Get mnemonic (assuming HD wallet support)
   drachma-cli --datadir=~/.drachma-test --testnet dumphdmnemonic > seed.txt
   
   # Record it securely
   cat seed.txt
   # Write down on paper for this test
   ```

3. **Get initial address and fund it:**
   ```bash
   # Get address
   ADDRESS=$(drachma-cli --datadir=~/.drachma-test --testnet getnewaddress)
   echo "Test address: $ADDRESS"
   
   # Fund from testnet faucet or mining
   # Record transaction ID
   ```

4. **Wait for confirmation:**
   ```bash
   # Check balance
   drachma-cli --datadir=~/.drachma-test --testnet getbalance
   ```

5. **Record wallet state:**
   ```bash
   # Get all addresses
   drachma-cli --datadir=~/.drachma-test --testnet listreceivedbyaddress 0 true > addresses_before.txt
   
   # Get transactions
   drachma-cli --datadir=~/.drachma-test --testnet listtransactions > txs_before.txt
   ```

6. **Destroy wallet:**
   ```bash
   drachma-cli --datadir=~/.drachma-test --testnet stop
   rm -rf ~/.drachma-test
   ```

7. **Recover from seed:**
   ```bash
   # Start fresh node
   drachma-node --datadir=~/.drachma-test --testnet &
   sleep 5
   
   # Import mnemonic
   SEED=$(cat seed.txt)
   drachma-cli --datadir=~/.drachma-test --testnet importhdmnemonic "$SEED"
   ```

8. **Verify recovery:**
   ```bash
   # Rescan blockchain
   drachma-cli --datadir=~/.drachma-test --testnet rescanblockchain
   
   # Check balance matches
   drachma-cli --datadir=~/.drachma-test --testnet getbalance
   
   # Compare addresses
   drachma-cli --datadir=~/.drachma-test --testnet listreceivedbyaddress 0 true > addresses_after.txt
   diff addresses_before.txt addresses_after.txt
   
   # Compare transactions
   drachma-cli --datadir=~/.drachma-test --testnet listtransactions > txs_after.txt
   diff txs_before.txt txs_after.txt
   ```

**Expected Result:** ✓ All addresses and transactions recovered

**Test Record:**
```
Date: ___________
Tester: ___________
Result: PASS / FAIL
Balance Before: ___________
Balance After: ___________
Notes: ___________
```

---

### Test 2: Seed Recovery with Gap Limit

**Objective:** Verify recovery works even with large address gaps.

**Procedure:**

1. Create wallet and generate 100 addresses without using them
2. Fund address #50
3. Record state
4. Destroy and recover wallet
5. Verify address #50 is found

**Expected Result:** ✓ Address found despite gap

---

### Test 3: Cross-Platform Seed Recovery

**Objective:** Verify seed works across different platforms.

**Procedure:**

1. Create wallet on Linux
2. Export seed
3. Import seed on macOS/Windows
4. Verify recovery

**Expected Result:** ✓ Wallet recovers on all platforms

---

## Encrypted Wallet Recovery

### Test 4: Encrypted wallet.dat Recovery

**Objective:** Verify encrypted wallet.dat can be recovered.

**Procedure:**

1. **Create and encrypt wallet:**
   ```bash
   drachma-cli --testnet encryptwallet "TestPassword123!"
   # Node will restart
   sleep 5
   ```

2. **Fund wallet:**
   ```bash
   ADDRESS=$(drachma-cli --testnet getnewaddress)
   # Send testnet coins to address
   ```

3. **Backup wallet:**
   ```bash
   drachma-cli --testnet backupwallet ~/wallet-backup.dat
   
   # Verify backup
   ls -lh ~/wallet-backup.dat
   sha256sum ~/wallet-backup.dat > wallet-backup.sha256
   ```

4. **Record state:**
   ```bash
   drachma-cli --testnet getwalletinfo > walletinfo_before.txt
   ```

5. **Simulate loss:**
   ```bash
   drachma-cli --testnet stop
   rm ~/.drachma/wallet.dat
   ```

6. **Restore from backup:**
   ```bash
   cp ~/wallet-backup.dat ~/.drachma/wallet.dat
   drachma-node --testnet &
   sleep 5
   ```

7. **Unlock and verify:**
   ```bash
   # Unlock wallet
   drachma-cli --testnet walletpassphrase "TestPassword123!" 600
   
   # Check balance
   drachma-cli --testnet getbalance
   
   # Compare state
   drachma-cli --testnet getwalletinfo > walletinfo_after.txt
   diff walletinfo_before.txt walletinfo_after.txt
   ```

**Expected Result:** ✓ Wallet fully recovered with correct passphrase

---

### Test 5: Wrong Passphrase Handling

**Objective:** Verify incorrect passphrase is properly rejected.

**Procedure:**

1. Use encrypted wallet from Test 4
2. Try wrong passphrases:
   ```bash
   drachma-cli --testnet walletpassphrase "WrongPassword" 600
   ```

**Expected Result:** ✓ Error message, wallet stays locked

---

## Multi-Asset Wallet Recovery

### Test 6: Multi-Asset Recovery

**Objective:** Verify all assets (TLN, DRM, OBL) are recovered.

**Procedure:**

1. **Create multi-asset wallet:**
   ```bash
   # Get addresses for each asset
   TLN_ADDR=$(drachma-cli --testnet getnewaddress "TLN" TLN)
   DRM_ADDR=$(drachma-cli --testnet getnewaddress "DRM" DRM)
   OBL_ADDR=$(drachma-cli --testnet getnewaddress "OBL" OBL)
   ```

2. **Fund each asset:**
   ```bash
   # Fund TLN, DRM, OBL separately
   # Record balances
   drachma-cli --testnet getbalances > balances_before.txt
   ```

3. **Backup seed:**
   ```bash
   drachma-cli --testnet dumphdmnemonic > multi-asset-seed.txt
   ```

4. **Destroy and recover:**
   ```bash
   # Stop and remove wallet
   drachma-cli --testnet stop
   rm -rf ~/.drachma
   
   # Start fresh and import seed
   drachma-node --testnet &
   sleep 5
   SEED=$(cat multi-asset-seed.txt)
   drachma-cli --testnet importhdmnemonic "$SEED"
   
   # Rescan
   drachma-cli --testnet rescanblockchain
   ```

5. **Verify all assets:**
   ```bash
   # Check balances
   drachma-cli --testnet getbalances > balances_after.txt
   diff balances_before.txt balances_after.txt
   ```

**Expected Result:** ✓ All three assets recovered correctly

---

## Edge Cases

### Test 7: Corrupted Wallet Recovery

**Objective:** Test recovery when wallet is corrupted.

**Procedure:**

1. Create and backup wallet
2. Intentionally corrupt wallet.dat (modify random bytes)
3. Attempt to open
4. Recover from backup

**Expected Result:** ✓ Backup allows recovery from corruption

---

### Test 8: Partial Wallet Recovery

**Objective:** Test recovery of partial wallet state.

**Procedure:**

1. Create wallet with transactions
2. Backup wallet
3. Perform more transactions (don't backup)
4. Restore from old backup
5. Rescan blockchain

**Expected Result:** ✓ Old backup recovers, rescan finds newer transactions

---

### Test 9: Recovery Time Test

**Objective:** Measure recovery time with large transaction history.

**Procedure:**

1. Create wallet with 1000+ transactions
2. Time full recovery process
3. Document time for rescan

**Expected Result:** Record recovery times for performance baseline

---

## Automated Testing

### Recovery Test Script

Create automated test:

```bash
#!/bin/bash
# test-wallet-recovery.sh

DATADIR=~/.drachma-test-$(date +%s)

echo "Starting wallet recovery test..."

# Test 1: Create wallet
echo "Creating wallet..."
drachma-node --datadir=$DATADIR --testnet &
PID=$!
sleep 10

# Test 2: Generate seed
echo "Generating seed..."
SEED=$(drachma-cli --datadir=$DATADIR --testnet dumphdmnemonic)
echo "Seed: $SEED"

# Test 3: Get address and balance
ADDR=$(drachma-cli --datadir=$DATADIR --testnet getnewaddress)
echo "Address: $ADDR"

BALANCE_BEFORE=$(drachma-cli --datadir=$DATADIR --testnet getbalance)
echo "Balance before: $BALANCE_BEFORE"

# Test 4: Destroy wallet
echo "Destroying wallet..."
drachma-cli --datadir=$DATADIR --testnet stop
rm -rf $DATADIR

# Test 5: Recover from seed
echo "Recovering from seed..."
drachma-node --datadir=$DATADIR --testnet &
sleep 10

drachma-cli --datadir=$DATADIR --testnet importhdmnemonic "$SEED"
drachma-cli --datadir=$DATADIR --testnet rescanblockchain

# Test 6: Verify recovery
BALANCE_AFTER=$(drachma-cli --datadir=$DATADIR --testnet getbalance)
echo "Balance after: $BALANCE_AFTER"

if [ "$BALANCE_BEFORE" == "$BALANCE_AFTER" ]; then
    echo "✓ Recovery test PASSED"
    exit 0
else
    echo "✗ Recovery test FAILED"
    exit 1
fi
```

---

## Test Checklist

Before mainnet launch, complete all tests:

- [ ] Test 1: Basic Seed Phrase Recovery
- [ ] Test 2: Seed Recovery with Gap Limit
- [ ] Test 3: Cross-Platform Seed Recovery
- [ ] Test 4: Encrypted wallet.dat Recovery
- [ ] Test 5: Wrong Passphrase Handling
- [ ] Test 6: Multi-Asset Recovery
- [ ] Test 7: Corrupted Wallet Recovery
- [ ] Test 8: Partial Wallet Recovery
- [ ] Test 9: Recovery Time Test
- [ ] Automated recovery test script
- [ ] Cross-platform testing (Linux/Mac/Windows)
- [ ] Different wallet versions compatibility

---

## Best Practices

**For Testers:**
1. Document everything
2. Test on testnet first
3. Verify checksums
4. Time each operation
5. Note any errors

**For Users:**
1. Always backup seed phrase
2. Store backups securely
3. Test recovery process
4. Never share seed with anyone
5. Use strong passphrases

---

## Reporting Issues

If any test fails:

1. **Record exact steps to reproduce**
2. **Save all error messages**
3. **Note environment:** OS, version, wallet version
4. **Report to:** GitHub Issues with tag "wallet-recovery"

---

## Success Criteria

All tests must pass before mainnet launch:
- ✓ 100% recovery rate from seed
- ✓ 100% recovery rate from backup
- ✓ All assets recovered correctly
- ✓ Recovery time acceptable (<30 min for rescan)
- ✓ Works across all supported platforms

---

**Last Updated:** January 6, 2026

For latest test procedures, refer to repository documentation.
