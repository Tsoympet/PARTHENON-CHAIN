# DRACHMA Testing Procedures

**Version:** 1.0  
**Last Updated:** January 6, 2026  
**Purpose:** Comprehensive guide for testing DRACHMA before mainnet launch

This document consolidates all testing procedures and provides a structured approach to validating the blockchain before mainnet deployment.

---

## Overview

Testing is critical for mainnet readiness. This document provides:
- Pre-launch testing checklist
- Testing methodology
- Links to specific test procedures
- Success criteria

---

## Testing Categories

### 1. Build & Installation Testing

**Document:** `docs/BUILD-PROCESS.md`

**Quick Test:**
```bash
./scripts/verify-build-test.sh
```

**Checklist:**
- [ ] Clean build succeeds on all platforms
- [ ] All dependencies resolve correctly
- [ ] Binaries execute without errors
- [ ] Version information correct

---

### 2. Genesis & Supply Verification

**Script:** `scripts/verify-genesis.sh`

**Quick Test:**
```bash
python3 ./scripts/verify-genesis.sh
```

**Checklist:**
- [ ] Genesis parameters match across configs
- [ ] Supply caps correct (TLN: 21M, DRM: 41M, OBL: 61M)
- [ ] Halving interval correct (2,102,400 blocks)
- [ ] Multi-asset configuration valid

---

### 3. Unit & Integration Tests

**Quick Test:**
```bash
make test
# or
ctest --test-dir build --output-on-failure
```

**Checklist:**
- [ ] All 97+ tests passing
- [ ] No skipped tests
- [ ] No memory leaks (valgrind if available)
- [ ] Test coverage acceptable

**Test Categories:**
- Consensus validation
- Crypto (Schnorr signatures)
- Merkle tree operations
- Transaction validation
- Block validation
- Fork resolution
- P2P messaging
- Mempool management
- WASM execution
- Asset isolation

---

### 4. Extended Validation

**Script:** `scripts/extended-validation.py`

**Quick Test:**
```bash
python3 ./scripts/extended-validation.py --testnet
```

**Checklist:**
- [ ] Node starts and syncs
- [ ] Network connectivity established
- [ ] Wallet operations functional
- [ ] Mining info accessible
- [ ] RPC endpoints responding

---

### 5. Wallet Recovery Testing

**Document:** `docs/WALLET-RECOVERY-TESTING.md`

**Test Areas:**
- [ ] Seed phrase recovery (24-word mnemonic)
- [ ] Encrypted wallet.dat recovery
- [ ] Multi-asset wallet recovery
- [ ] Cross-platform recovery
- [ ] Edge cases (corruption, gaps, etc.)

**Success Criteria:**
- 100% recovery rate from valid backups
- All assets recovered correctly
- Works across all platforms

---

### 6. RPC Testing

**Document:** `docs/RPC-TESTING.md`

**Quick Test:**
```bash
python3 ./scripts/test-rpc.py
```

**Categories:**
- [ ] Blockchain RPCs (getblock, getblockchaininfo, etc.)
- [ ] Network RPCs (getpeerinfo, getnetworkinfo, etc.)
- [ ] Mining RPCs (getmininginfo, submitblock, etc.)
- [ ] Wallet RPCs (getbalance, sendtoaddress, etc.)
- [ ] Transaction RPCs (getrawtransaction, etc.)
- [ ] Performance (no linear scan issues)
- [ ] Security (input validation, auth)

---

### 7. Network Testing

**Testnet Validation:**

```bash
# Start testnet node
drachma-node --testnet

# Monitor
watch -n 5 'drachma-cli --testnet getblockchaininfo'
```

**Checklist:**
- [ ] Connects to peers
- [ ] Syncs blockchain
- [ ] Propagates transactions
- [ ] Propagates blocks
- [ ] Handles network partitions
- [ ] Recovers from disconnection

---

### 8. Mining Testing

**Document:** `docs/MINING-GUIDE.md`

**Test Scenarios:**
- [ ] CPU mining functional
- [ ] GPU mining functional
- [ ] Pool mining (Stratum)
- [ ] Block submission accepted
- [ ] Orphan rate acceptable
- [ ] Multi-asset mining (TLN vs DRM)

---

### 9. Stress Testing

**Long-Running Tests:**

```bash
#!/bin/bash
# Run for 24+ hours

# Monitor
while true; do
    echo "=== $(date) ==="
    drachma-cli --testnet getblockchaininfo
    drachma-cli --testnet getmempoolinfo
    drachma-cli --testnet getpeerinfo | jq length
    sleep 300
done
```

**Checklist:**
- [ ] 24-hour stability test
- [ ] No memory leaks
- [ ] No crashes
- [ ] Performance stable
- [ ] Database size growth acceptable

---

### 10. Security Testing

**Basic Security Checks:**

```bash
# Check for common issues
grep -r "TODO.*security" .
grep -r "FIXME.*vuln" .

# RPC authentication
curl http://127.0.0.1:9332/  # Should fail without auth

# Input validation
./scripts/test-rpc.py  # Includes security tests
```

**Checklist:**
- [ ] RPC authentication enforced
- [ ] Input validation present
- [ ] No SQL/command injection
- [ ] Safe error handling
- [ ] No secrets in logs

**External Audit:**
- [ ] Security audit scheduled (LAUNCH-ACTION-ITEMS.md #1)
- [ ] All critical findings addressed
- [ ] Audit report published

---

## Pre-Launch Testing Timeline

### 8-10 Weeks Before Launch

- [ ] Complete all unit tests
- [ ] Verify genesis parameters
- [ ] Basic RPC testing
- [ ] Wallet creation/recovery

### 6-8 Weeks Before Launch

- [ ] Extended testnet validation
- [ ] Multi-day stability tests
- [ ] Performance benchmarking
- [ ] Cross-platform testing

### 4-6 Weeks Before Launch

- [ ] Security audit initiated
- [ ] Stress testing
- [ ] Pool mining tests
- [ ] Multi-asset testing

### 2-4 Weeks Before Launch

- [ ] Audit findings addressed
- [ ] Final testnet validation
- [ ] Release candidate testing
- [ ] Documentation review

### 1-2 Weeks Before Launch

- [ ] Code freeze (critical fixes only)
- [ ] Final security review
- [ ] All checklists complete
- [ ] Launch readiness confirmed

---

## Testing Best Practices

### General Guidelines

1. **Test on testnet first:** Never test on mainnet
2. **Document everything:** Record all test results
3. **Automate when possible:** Use scripts for repeatability
4. **Test edge cases:** Not just happy path
5. **Cross-platform:** Test on Linux, macOS, Windows

### Test Data Management

```bash
# Use isolated test directories
export DRACHMA_DATADIR=~/.drachma-test-$(date +%s)

# Clean up after tests
rm -rf $DRACHMA_DATADIR
```

### Logging

```bash
# Enable debug logging for tests
echo "debug=1" >> $DRACHMA_DATADIR/drachma.conf

# Review logs
tail -f $DRACHMA_DATADIR/debug.log
```

---

## Automated Test Execution

### Master Test Script

```bash
#!/bin/bash
# run-all-tests.sh - Execute all test suites

set -e

echo "DRACHMA Comprehensive Test Suite"
echo "=================================="

# 1. Build verification
echo "1. Build verification..."
./scripts/verify-build-test.sh || exit 1

# 2. Genesis verification
echo "2. Genesis verification..."
python3 ./scripts/verify-genesis.sh || exit 1

# 3. Unit tests
echo "3. Unit tests..."
make test || exit 1

# 4. Extended validation (requires running node)
echo "4. Extended validation..."
# Start node in background
drachma-node --testnet &
NODE_PID=$!
sleep 30

python3 ./scripts/extended-validation.py --testnet || {
    kill $NODE_PID
    exit 1
}

# 5. RPC tests
echo "5. RPC tests..."
python3 ./scripts/test-rpc.py || {
    kill $NODE_PID
    exit 1
}

# Cleanup
drachma-cli --testnet stop
wait $NODE_PID

echo ""
echo "=================================="
echo "✓ All tests passed!"
echo "=================================="
```

---

## Test Results Tracking

### Test Report Template

```markdown
# Test Report - DRACHMA

**Date:** YYYY-MM-DD
**Tester:** Name
**Version:** vX.Y.Z
**Platform:** OS/Architecture

## Summary
- Total Tests: X
- Passed: Y
- Failed: Z
- Skipped: W

## Build Testing
- [ ] Clean build: PASS/FAIL
- [ ] Dependencies: PASS/FAIL
- Notes: ...

## Genesis Verification
- [ ] Parameters: PASS/FAIL
- [ ] Supply caps: PASS/FAIL
- Notes: ...

## Unit Tests
- [ ] All tests: PASS/FAIL (X/Y)
- Failed tests: ...
- Notes: ...

## Extended Validation
- [ ] Node startup: PASS/FAIL
- [ ] Network: PASS/FAIL
- [ ] Wallet: PASS/FAIL
- Notes: ...

## RPC Testing
- [ ] All endpoints: PASS/FAIL
- [ ] Performance: PASS/FAIL
- [ ] Security: PASS/FAIL
- Notes: ...

## Issues Found
1. ...
2. ...

## Recommendations
- ...
```

---

## CI/CD Integration

### GitHub Actions

Tests run automatically on:
- Every commit (unit tests)
- Pull requests (full suite)
- Release tags (comprehensive validation)

See `.github/workflows/ci.yml` for details.

### Local CI Simulation

```bash
# Run same tests as CI
act -j test
```

---

## Success Criteria

### Minimum Requirements for Launch

**All must be ✓ before mainnet:**

- [ ] 100% unit tests passing
- [ ] Genesis parameters verified
- [ ] Build succeeds on all platforms
- [ ] No critical security issues
- [ ] RPC layer functional
- [ ] Wallet recovery tested
- [ ] Multi-day stability test passed
- [ ] External audit completed
- [ ] All high/critical audit findings resolved

### Performance Benchmarks

- Node sync time: < 24 hours for full chain
- RPC response time: < 1 second average
- Memory usage: < 2GB for node
- CPU usage: < 50% during sync

---

## Getting Help

**Issues during testing?**

1. Check `docs/TROUBLESHOOTING.md`
2. Review test-specific documentation
3. Search GitHub Issues
4. Report new issues with:
   - Test that failed
   - Expected vs actual result
   - Environment details
   - Steps to reproduce

---

## Quick Reference

**Run all tests:**
```bash
./run-all-tests.sh
```

**Individual test categories:**
```bash
./scripts/verify-build-test.sh
python3 ./scripts/verify-genesis.sh
make test
python3 ./scripts/extended-validation.py --testnet
python3 ./scripts/test-rpc.py
```

**View test results:**
```bash
ctest --test-dir build --output-on-failure --verbose
```

---

## Related Documents

- `ANALYSIS-EXECUTIVE-SUMMARY.md` - Overall readiness assessment
- `docs/LAUNCH-ACTION-ITEMS.md` - Pre-launch checklist
- `docs/MAINNET-READINESS.md` - Technical readiness
- `docs/TROUBLESHOOTING.md` - Common issues
- `docs/BUILD-PROCESS.md` - Build instructions
- `docs/WALLET-RECOVERY-TESTING.md` - Wallet tests
- `docs/RPC-TESTING.md` - RPC tests

---

**Last Updated:** January 6, 2026

Testing is never truly "complete" - continue testing even after mainnet launch!
