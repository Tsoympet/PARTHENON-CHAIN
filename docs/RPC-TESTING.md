# RPC Endpoint Validation Procedures

**Version:** 1.0  
**Last Updated:** January 6, 2026  
**Purpose:** Document procedures for validating all RPC endpoints

This document provides test procedures for validating RPC functionality, particularly important given the RPC hardening work identified in LAUNCH-ACTION-ITEMS.md.

---

## Overview

The RPC (Remote Procedure Call) layer is the primary interface for:
- Wallet operations
- Node queries
- Mining operations
- Network monitoring

**Known Issues (from AUDIT.md):**
- Linear block scanning (performance concern)
- Unbounded JSON parser (security concern)
- Need for indexed storage

These tests help validate RPC functionality and identify issues before mainnet launch.

---

## Test Environment

### Setup

```bash
# Start testnet node with RPC enabled
cat > ~/.drachma/drachma.conf << EOF
testnet=1
server=1
rpcuser=test
rpcpassword=testpass123
rpcallowip=127.0.0.1
rpcbind=127.0.0.1:19332
EOF

drachma-node --testnet
```

### Test Tools

**CLI:**
```bash
alias drpc='drachma-cli --testnet'
```

**curl:**
```bash
RPC_URL="http://test:testpass123@127.0.0.1:19332"
alias drpc-curl='curl -s --user test:testpass123 --data-binary'
```

**Python:**
```python
import requests
import json

def rpc_call(method, params=[]):
    payload = {
        "jsonrpc": "1.0",
        "id": "test",
        "method": method,
        "params": params
    }
    response = requests.post(
        "http://127.0.0.1:19332",
        auth=("test", "testpass123"),
        json=payload
    )
    return response.json()
```

---

## RPC Categories

### 1. Blockchain RPCs

#### getblockchaininfo
```bash
# Test basic call
drpc getblockchaininfo

# Expected fields:
# - chain, blocks, headers, bestblockhash
# - difficulty, mediantime, verificationprogress
```

**Validation:**
- [ ] Returns valid JSON
- [ ] All expected fields present
- [ ] blocks <= headers (sync indicator)
- [ ] chain matches network (test/main)

#### getblockcount
```bash
drpc getblockcount

# Expected: Integer >= 0
```

**Validation:**
- [ ] Returns integer
- [ ] Value matches getblockchaininfo.blocks

#### getbestblockhash
```bash
HASH=$(drpc getbestblockhash)
echo $HASH

# Expected: 64-character hex string
```

**Validation:**
- [ ] Returns 64-char hex
- [ ] Matches getblockchaininfo.bestblockhash

#### getblock
```bash
# Get block by hash
drpc getblock $HASH

# Get block with verbosity
drpc getblock $HASH 0  # Raw hex
drpc getblock $HASH 1  # JSON (default)
drpc getblock $HASH 2  # JSON with tx details
```

**Validation:**
- [ ] Valid hash returns block data
- [ ] Invalid hash returns error
- [ ] Verbosity levels work correctly
- [ ] Required fields present (height, tx, time, etc.)

#### getblockhash
```bash
# Get hash at specific height
drpc getblockhash 0     # Genesis
drpc getblockhash 100
drpc getblockhash 999999  # Should error
```

**Validation:**
- [ ] Valid height returns hash
- [ ] Height > chain tip returns error
- [ ] Negative height returns error

#### getchaintips
```bash
drpc getchaintips
```

**Validation:**
- [ ] Returns array of tips
- [ ] Active chain marked correctly

---

### 2. Network RPCs

#### getnetworkinfo
```bash
drpc getnetworkinfo
```

**Validation:**
- [ ] version, subversion present
- [ ] protocolversion valid
- [ ] connections count valid
- [ ] networkactive boolean

#### getpeerinfo
```bash
drpc getpeerinfo
```

**Validation:**
- [ ] Returns array (may be empty)
- [ ] Each peer has: addr, version, conntime, etc.

#### getconnectioncount
```bash
drpc getconnectioncount
```

**Validation:**
- [ ] Returns integer >= 0
- [ ] Matches length of getpeerinfo array

#### addnode / removenode
```bash
# Add node
drpc addnode "seed.example.com:9333" "add"

# Remove node
drpc removenode "seed.example.com:9333"

# One-shot connect
drpc addnode "seed.example.com:9333" "onetry"
```

**Validation:**
- [ ] Invalid action returns error
- [ ] Invalid address format returns error

---

### 3. Mining RPCs

#### getmininginfo
```bash
drpc getmininginfo
```

**Validation:**
- [ ] blocks, difficulty present
- [ ] networkhashps present
- [ ] Asset-specific info if applicable

#### getnetworkhashps
```bash
drpc getnetworkhashps
drpc getnetworkhashps 120  # Last 120 blocks
drpc getnetworkhashps 120 100  # Blocks 100-220
```

**Validation:**
- [ ] Returns number (hashrate)
- [ ] Parameters affect result appropriately

#### getblocktemplate
```bash
# Requires wallet or address
drpc getblocktemplate '{"rules": ["segwit"]}'
```

**Validation:**
- [ ] Returns template with required fields
- [ ] version, previousblockhash, transactions present
- [ ] coinbasevalue, target correct

#### submitblock
```bash
# Submit mined block (hex)
drpc submitblock "block_hex_data"
```

**Validation:**
- [ ] Invalid block returns specific error
- [ ] Valid block accepted (if valid PoW)

---

### 4. Wallet RPCs

#### getwalletinfo
```bash
drpc getwalletinfo
```

**Validation:**
- [ ] walletversion, balance, txcount present
- [ ] encrypted boolean correct

#### getnewaddress
```bash
# Default
ADDR=$(drpc getnewaddress)

# With label
drpc getnewaddress "my-label"

# With asset (if multi-asset)
drpc getnewaddress "label" "DRM"
```

**Validation:**
- [ ] Returns valid address format
- [ ] Each call returns unique address

#### validateaddress
```bash
drpc validateaddress $ADDR
```

**Validation:**
- [ ] isvalid true for valid address
- [ ] isvalid false for invalid address
- [ ] ismine correct for owned addresses

#### getbalance
```bash
drpc getbalance
drpc getbalance "*" 1  # Min 1 confirmation
drpc getbalance "label"  # Specific label
```

**Validation:**
- [ ] Returns numeric value >= 0
- [ ] Confirmations parameter works

#### getbalances (multi-asset)
```bash
drpc getbalances
```

**Validation:**
- [ ] Returns all asset balances
- [ ] TLN, DRM, OBL present (if applicable)

#### listtransactions
```bash
drpc listtransactions
drpc listtransactions "*" 10  # Last 10
drpc listtransactions "*" 10 5  # Skip 5, return 10
```

**Validation:**
- [ ] Returns array of transactions
- [ ] Each has: txid, amount, confirmations, etc.
- [ ] Pagination works correctly

#### sendtoaddress
```bash
# Send amount (testnet only!)
drpc sendtoaddress $ADDR 1.5
drpc sendtoaddress $ADDR 1.5 "comment" "to-comment"
```

**Validation:**
- [ ] Returns transaction ID
- [ ] Transaction appears in listtransactions
- [ ] Balance decreases appropriately
- [ ] Invalid address returns error
- [ ] Insufficient funds returns error

---

### 5. Transaction RPCs

#### gettransaction
```bash
TXID="transaction_id_here"
drpc gettransaction $TXID
```

**Validation:**
- [ ] Returns transaction details
- [ ] confirmations, blockhash present
- [ ] Invalid txid returns error

#### getrawtransaction
```bash
drpc getrawtransaction $TXID
drpc getrawtransaction $TXID 1  # Verbose
```

**Validation:**
- [ ] Returns hex or JSON
- [ ] Verbose includes decoded details

#### decoderawtransaction
```bash
RAW_TX="hex_tx_data"
drpc decoderawtransaction $RAW_TX
```

**Validation:**
- [ ] Returns decoded transaction
- [ ] Invalid hex returns error

#### createrawtransaction
```bash
# Create transaction
drpc createrawtransaction '[{"txid":"...","vout":0}]' '{"addr":"1.0"}'
```

**Validation:**
- [ ] Returns raw transaction hex
- [ ] Invalid inputs return error

#### signrawtransaction
```bash
drpc signrawtransactionwithwallet $RAW_TX
```

**Validation:**
- [ ] Returns signed transaction
- [ ] complete field indicates success

#### sendrawtransaction
```bash
drpc sendrawtransaction $SIGNED_TX
```

**Validation:**
- [ ] Returns txid if broadcast successful
- [ ] Invalid tx returns specific error

---

### 6. Utility RPCs

#### uptime
```bash
drpc uptime
```

**Validation:**
- [ ] Returns seconds since node start
- [ ] Increases over time

#### help
```bash
drpc help
drpc help getblock
```

**Validation:**
- [ ] Lists all commands
- [ ] Command-specific help available

#### stop
```bash
drpc stop
```

**Validation:**
- [ ] Node shuts down gracefully
- [ ] Can restart successfully

---

## Performance Testing

### Block Retrieval Performance

Test linear scan issue (AUDIT.md concern):

```bash
#!/bin/bash
# Test block retrieval performance

for i in 0 100 1000 10000; do
    echo "Testing block $i..."
    time drpc getblock $(drpc getblockhash $i) > /dev/null
done
```

**Expected:** Time should not increase linearly with block height (after RPC hardening).

### Large Result Sets

Test JSON parser limits:

```bash
# Large transaction list
time drpc listtransactions "*" 10000

# Many peers (if possible)
time drpc getpeerinfo
```

**Validation:**
- [ ] No crashes with large results
- [ ] Memory usage remains reasonable

### Concurrent Requests

```bash
#!/bin/bash
# Concurrent RPC test

for i in {1..100}; do
    drpc getblockcount &
done
wait
```

**Validation:**
- [ ] All requests complete successfully
- [ ] No crashes or hangs

---

## Security Testing

### Authentication

```bash
# Wrong credentials
curl --user wrong:password --data-binary \
  '{"jsonrpc":"1.0","id":"test","method":"getblockcount","params":[]}' \
  -H 'content-type: text/plain;' \
  http://127.0.0.1:19332/

# Expected: 401 Unauthorized
```

### Input Validation

```bash
# Malformed JSON
echo 'not json' | drpc-curl

# Oversized payload (test after hardening)
python3 << 'EOF'
import requests
payload = "x" * 10000000  # 10MB
response = requests.post(
    "http://127.0.0.1:19332",
    auth=("test", "testpass123"),
    data=payload
)
print(response.status_code)
EOF
```

**Expected:** Graceful rejection, not crash

### SQL Injection / Command Injection

```bash
# Test with malicious inputs
drpc getblock "'; DROP TABLE blocks; --"
drpc getnewaddress "$(rm -rf /)"
```

**Expected:** Input sanitized, no execution

---

## Automated Test Suite

### RPC Test Script

```python
#!/usr/bin/env python3
"""
Comprehensive RPC test suite
"""

import requests
import json
import sys

RPC_URL = "http://127.0.0.1:19332"
RPC_AUTH = ("test", "testpass123")

def rpc(method, params=[]):
    """Make RPC call"""
    payload = {
        "jsonrpc": "1.0",
        "id": "test",
        "method": method,
        "params": params
    }
    try:
        r = requests.post(RPC_URL, auth=RPC_AUTH, json=payload, timeout=30)
        return r.json()
    except Exception as e:
        return {"error": str(e)}

def test_rpc(name, method, params=[], expect_error=False):
    """Test single RPC call"""
    print(f"Testing {name}...", end=" ")
    result = rpc(method, params)
    
    if expect_error:
        if "error" in result and result["error"]:
            print("✓ (error expected)")
            return True
        else:
            print("✗ (expected error)")
            return False
    else:
        if "result" in result and result.get("error") is None:
            print("✓")
            return True
        else:
            print(f"✗ {result.get('error', 'unknown error')}")
            return False

def main():
    print("DRACHMA RPC Test Suite")
    print("=" * 60)
    
    tests_passed = 0
    tests_total = 0
    
    # Blockchain RPCs
    print("\n1. Blockchain RPCs")
    tests = [
        ("getblockchaininfo", "getblockchaininfo", []),
        ("getblockcount", "getblockcount", []),
        ("getbestblockhash", "getbestblockhash", []),
        ("getblockhash(0)", "getblockhash", [0]),
        ("getblockhash(invalid)", "getblockhash", [-1], True),
    ]
    
    for name, method, params, *expect_err in tests:
        tests_total += 1
        if test_rpc(name, method, params, bool(expect_err)):
            tests_passed += 1
    
    # Network RPCs
    print("\n2. Network RPCs")
    tests = [
        ("getnetworkinfo", "getnetworkinfo", []),
        ("getpeerinfo", "getpeerinfo", []),
        ("getconnectioncount", "getconnectioncount", []),
    ]
    
    for name, method, params in tests:
        tests_total += 1
        if test_rpc(name, method, params):
            tests_passed += 1
    
    # Mining RPCs
    print("\n3. Mining RPCs")
    tests = [
        ("getmininginfo", "getmininginfo", []),
        ("getnetworkhashps", "getnetworkhashps", []),
    ]
    
    for name, method, params in tests:
        tests_total += 1
        if test_rpc(name, method, params):
            tests_passed += 1
    
    # Utility RPCs
    print("\n4. Utility RPCs")
    tests = [
        ("uptime", "uptime", []),
        ("help", "help", []),
    ]
    
    for name, method, params in tests:
        tests_total += 1
        if test_rpc(name, method, params):
            tests_passed += 1
    
    # Summary
    print("\n" + "=" * 60)
    print(f"Results: {tests_passed}/{tests_total} tests passed")
    
    if tests_passed == tests_total:
        print("✓ All tests passed")
        return 0
    else:
        print(f"✗ {tests_total - tests_passed} tests failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
```

Save as `scripts/test-rpc.py` and run:
```bash
chmod +x scripts/test-rpc.py
./scripts/test-rpc.py
```

---

## Test Checklist

Before mainnet launch:

- [ ] All blockchain RPCs tested
- [ ] All network RPCs tested
- [ ] All mining RPCs tested
- [ ] All wallet RPCs tested
- [ ] All transaction RPCs tested
- [ ] Performance tests completed
- [ ] Security tests passed
- [ ] Concurrent request handling verified
- [ ] Input validation confirmed
- [ ] Error handling appropriate
- [ ] Documentation matches implementation

---

## Known Issues Tracking

Document any issues found:

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|
| Linear block scan | Medium | Planned fix | LAUNCH-ACTION-ITEMS.md #3 |
| Unbounded JSON | High | Planned fix | LAUNCH-ACTION-ITEMS.md #3 |
| ... | ... | ... | ... |

---

## Success Criteria

For mainnet launch:
- ✓ All critical RPCs functional
- ✓ Performance acceptable (< 1s for most calls)
- ✓ No crashes with valid inputs
- ✓ Graceful error handling
- ✓ Security hardening complete (per LAUNCH-ACTION-ITEMS.md)

---

**Last Updated:** January 6, 2026

Refer to LAUNCH-ACTION-ITEMS.md for RPC hardening progress.
