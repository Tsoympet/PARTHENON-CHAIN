# DRACHMA Troubleshooting Guide

**Version:** 1.0  
**Last Updated:** January 6, 2026  
**Purpose:** Help users resolve common issues with DRACHMA blockchain

This guide addresses common problems encountered when building, running, and using the DRACHMA blockchain.

---

## Table of Contents

1. [Build Issues](#build-issues)
2. [Runtime Issues](#runtime-issues)
3. [Network Issues](#network-issues)
4. [Wallet Issues](#wallet-issues)
5. [Mining Issues](#mining-issues)
6. [General Debugging](#general-debugging)

---

## Build Issues

### CMake Configuration Fails

**Symptom:** `cmake` command fails with dependency errors

**Solutions:**

1. **Missing dependencies:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install build-essential cmake libssl-dev libboost-all-dev
   
   # macOS
   brew install cmake boost openssl
   
   # Fedora/RHEL
   sudo dnf install gcc-c++ cmake openssl-devel boost-devel
   ```

2. **CMake version too old:**
   ```bash
   cmake --version  # Should be 3.15 or higher
   ```
   If too old, install from [cmake.org](https://cmake.org/download/)

3. **Clear build cache and retry:**
   ```bash
   rm -rf build/
   make distclean
   make
   ```

### Compilation Errors

**Symptom:** Build fails with compiler errors

**Solutions:**

1. **Compiler too old:**
   - Requires GCC 9+ or Clang 10+
   - Check version: `g++ --version` or `clang++ --version`

2. **Out of memory:**
   - Reduce parallel jobs: `cmake --build build --parallel 2`
   - Or disable parallelism: `cmake --build build`

3. **Boost library issues:**
   ```bash
   # Ensure Boost 1.65+ is installed
   dpkg -l | grep libboost  # Debian/Ubuntu
   brew info boost          # macOS
   ```

### Test Failures

**Symptom:** `make test` or `ctest` shows failures

**Solutions:**

1. **Check for known test issues:**
   ```bash
   # Run tests verbosely to see details
   ctest --test-dir build --output-on-failure --verbose
   ```

2. **Timing-sensitive tests:**
   - Some tests may fail on slow systems
   - Try running individually: `ctest --test-dir build -R <test_name>`

3. **Clean rebuild:**
   ```bash
   make distclean
   make
   make test
   ```

### Coverage Tool Issues

**Symptom:** Error message about "coverage.py not installed" or coverage-related failures

**Solution:**

This project uses **gcovr** (C++ coverage tool), NOT Python's coverage.py. If you see coverage-related errors:

```bash
# Install gcovr (the correct tool for this C++ project)
sudo apt-get install gcovr  # Ubuntu/Debian
pip install gcovr           # Via pip (any OS)
```

**Common misconception:** coverage.py is a Python-specific code coverage tool and is not needed for this C++ blockchain project. The CI workflow and build system use gcovr to analyze C++ code coverage.

To generate coverage reports locally:
```bash
cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug -DDRACHMA_COVERAGE=ON
cmake --build build-cov --parallel
ctest --test-dir build-cov --output-on-failure
gcovr --root . --object-directory build-cov --print-summary
```

---

## Runtime Issues

### Node Won't Start

**Symptom:** `drachma-node` fails to start

**Solutions:**

1. **Check configuration:**
   ```bash
   # Verify config file syntax
   cat ~/.drachma/drachma.conf
   ```

2. **Port already in use:**
   ```bash
   # Check if port 9333 (mainnet) or 19333 (testnet) is in use
   netstat -tuln | grep 9333
   
   # Change port in config:
   echo "port=9334" >> ~/.drachma/drachma.conf
   ```

3. **Permission issues:**
   ```bash
   # Ensure data directory is writable
   ls -la ~/.drachma/
   chmod 700 ~/.drachma/
   ```

4. **Check logs:**
   ```bash
   tail -f ~/.drachma/debug.log
   ```

### Sync Issues

**Symptom:** Node not syncing or stuck at a block

**Solutions:**

1. **Verify network connectivity:**
   ```bash
   # Check if connected to peers
   drachma-cli getpeerinfo
   ```

2. **Bootstrap from checkpoint:**
   ```bash
   # Download latest bootstrap file (if available)
   # Extract to data directory
   ```

3. **Reindex blockchain:**
   ```bash
   drachma-node --reindex
   ```

4. **Clear peer database and resync:**
   ```bash
   drachma-node stop
   rm ~/.drachma/peers.dat
   drachma-node start
   ```

### High Memory Usage

**Symptom:** Node consuming excessive RAM

**Solutions:**

1. **Reduce cache size:**
   ```bash
   # Add to drachma.conf
   dbcache=300  # Default is 450MB
   ```

2. **Limit mempool size:**
   ```bash
   maxmempool=100  # MB
   ```

3. **Prune old blocks (if not mining/archival node):**
   ```bash
   prune=550  # Keep only last 550MB of blocks
   ```

---

## Network Issues

### Cannot Connect to Peers

**Symptom:** Node has 0 connections

**Solutions:**

1. **Firewall blocking connections:**
   ```bash
   # Allow inbound connections on port 9333
   sudo ufw allow 9333/tcp
   
   # Or for testnet
   sudo ufw allow 19333/tcp
   ```

2. **Add seed nodes manually:**
   ```bash
   # Add to drachma.conf
   addnode=seed1.drachma.network:9333
   addnode=seed2.drachma.network:9333
   ```

3. **Check DNS resolution:**
   ```bash
   nslookup seed.drachma.network
   ```

4. **Try testnet first:**
   ```bash
   drachma-node --testnet
   ```

### Slow Propagation

**Symptom:** Transactions/blocks take long to propagate

**Solutions:**

1. **Increase connection count:**
   ```bash
   # Add to drachma.conf
   maxconnections=125  # Default is 125
   ```

2. **Check bandwidth limits:**
   ```bash
   # Remove bandwidth restrictions if set
   # maxuploadtarget=<MiB per day>
   ```

3. **Verify not on fork:**
   ```bash
   drachma-cli getblockchaininfo
   # Check 'headers' matches network consensus
   ```

---

## Wallet Issues

### Wallet Won't Unlock

**Symptom:** Cannot unlock encrypted wallet

**Solutions:**

1. **Verify passphrase:**
   - Passphrases are case-sensitive
   - Check for extra spaces

2. **Corruption detection:**
   ```bash
   # Backup first!
   cp ~/.drachma/wallet.dat ~/wallet.dat.backup
   
   # Try wallet recovery
   drachma-cli salvage wallet
   ```

3. **Restore from seed:**
   - If you have your 24-word mnemonic, restore to new wallet
   ```bash
   drachma-cli importmnemonic "your 24 words here"
   ```

### Missing Transactions

**Symptom:** Expected transaction not showing

**Solutions:**

1. **Rescan blockchain:**
   ```bash
   drachma-cli rescanblockchain
   # Or on startup:
   drachma-node --rescan
   ```

2. **Check transaction status:**
   ```bash
   drachma-cli gettransaction <txid>
   ```

3. **Verify address derivation:**
   ```bash
   drachma-cli listreceivedbyaddress 0 true
   ```

### Transaction Stuck Unconfirmed

**Symptom:** Transaction pending for long time

**Solutions:**

1. **Check fee rate:**
   ```bash
   drachma-cli gettransaction <txid>
   # Compare fee rate to current mempool
   ```

2. **Replace-by-fee (if enabled):**
   ```bash
   drachma-cli bumpfee <txid>
   ```

3. **Wait or abandon:**
   ```bash
   # After 2 weeks, transaction may be dropped
   # Or explicitly abandon:
   drachma-cli abandontransaction <txid>
   ```

---

## Mining Issues

### Mining Not Working

**Symptom:** Miner runs but no blocks found

**Solutions:**

1. **Check hash rate:**
   ```bash
   # Verify miner is actually hashing
   # Monitor output for "hashrate: X MH/s"
   ```

2. **Network difficulty too high:**
   - Solo mining on mainnet requires significant hashrate
   - Consider joining a pool (when available)

3. **Connection to node:**
   ```bash
   # Verify RPC connection
   curl --user username:password --data-binary '{"jsonrpc":"1.0","id":"test","method":"getblockcount","params":[]}' -H 'content-type: text/plain;' http://127.0.0.1:9332/
   ```

4. **CPU/GPU miner configuration:**
   ```bash
   # CPU miner
   ./drachma-cpu-miner --threads 4 --intensity 20
   
   # GPU miner
   ./drachma-gpu-miner --device 0 --intensity 20
   ```

### Invalid Block Submissions

**Symptom:** Miner finds blocks but they're rejected

**Solutions:**

1. **Check node sync status:**
   ```bash
   drachma-cli getblockchaininfo
   # Ensure 'blocks' == 'headers'
   ```

2. **Verify block template:**
   - Ensure using latest `getblocktemplate`
   - Check for stale work

3. **Time synchronization:**
   ```bash
   # Install NTP
   sudo apt-get install ntp
   sudo systemctl start ntp
   
   # Verify system time
   date
   ```

---

## General Debugging

### Enable Debug Logging

```bash
# Add to drachma.conf
debug=1
debuglogfile=/path/to/custom/debug.log

# Or specific subsystems
debug=net
debug=mempool
debug=validation
```

### Check System Resources

```bash
# Disk space
df -h ~/.drachma/

# Memory usage
top -p $(pgrep drachma-node)

# Open files (should be < ulimit)
lsof -p $(pgrep drachma-node) | wc -l
```

### Verify Installation

```bash
# Check version
drachma-node --version

# Verify genesis
./scripts/verify-genesis.sh

# Test build
./scripts/verify-build-test.sh
```

### Common Config Parameters

```bash
# Example ~/.drachma/drachma.conf

# Network
listen=1
maxconnections=125

# RPC
server=1
rpcuser=yourusername
rpcpassword=yourpassword
rpcallowip=127.0.0.1

# Performance
dbcache=450
maxmempool=300

# Logging
debug=0
logtimestamps=1

# Testnet
testnet=0
```

---

## Getting More Help

If your issue isn't covered here:

1. **Check Documentation:**
   - `doc/getting-started/` - Setup guides
   - `doc/reference/` - Technical references
   - `doc/operators/` - Node operation

2. **Review Logs:**
   ```bash
   tail -n 100 ~/.drachma/debug.log
   ```

3. **Community Support:**
   - GitHub Issues: Report bugs or ask questions
   - Community forums/chat (if available)

4. **Security Issues:**
   - Review `.github/SECURITY.md`
   - Report privately to security contacts

---

## Known Issues

### Mainnet Launch Status

**Status:** NOT YET LAUNCHED

- Mainnet is not live yet
- Genesis block requires final mining
- See `ANALYSIS-EXECUTIVE-SUMMARY.md` for launch timeline
- Current focus: Security audit and pre-launch testing

### RPC Layer Performance

**Status:** Acknowledged limitation (AUDIT.md)

- Linear block scanning may be slow for large chains
- Hardening in progress (LAUNCH-ACTION-ITEMS.md)
- Use indexed queries when available

### GUI Assets

**Status:** Functional placeholders in place (AUDIT.md)

- Core icons: Functional placeholder SVGs available (app-icon, tray-icon, splash-icon)
- Asset icons: Basic placeholders for TLN/DRM/OBL
- Production-ready professional icons recommended before mainnet launch
- Release-ready installers in development
- Fully functional for testnet purposes

---

**Last Updated:** January 6, 2026  
**Feedback:** Please report issues or suggest improvements via GitHub Issues
