# DRACHMA Scripts Directory

This directory contains utility scripts for building, testing, and validating the DRACHMA blockchain.

---

## Verification Scripts

### verify-genesis.sh
**Purpose:** Verify genesis parameters and supply caps

**Note:** Despite the .sh extension, this is a Python script (for historical reasons).

**Usage:**
```bash
# Direct execution (uses shebang)
./scripts/verify-genesis.sh

# Or explicit Python invocation
python3 scripts/verify-genesis.sh
```

**Validates:**
- Genesis parameters consistency (mainnet/testnet)
- Supply caps: TLN (21M), DRM (41M), OBL (61M)
- Halving interval (2,102,400 blocks)
- Multi-asset configuration

**Documentation:** Part of LAUNCH-ACTION-ITEMS.md #2 (Genesis Block Finalization)

---

### verify-build-test.sh
**Purpose:** Comprehensive build and test verification

**Usage:**
```bash
./scripts/verify-build-test.sh

# Verbose mode
VERBOSE=1 ./scripts/verify-build-test.sh
```

**Validates:**
- Build dependencies installed
- CMake configuration succeeds
- Build completes without errors
- All tests pass
- Supply constants in code are correct

---

### extended-validation.py
**Purpose:** Live node validation and health checks

**Usage:**
```bash
# Testnet (default)
python3 scripts/extended-validation.py

# Mainnet
python3 scripts/extended-validation.py --mainnet
```

**Validates:**
- Node running and synced
- Network connectivity
- Wallet functionality
- Mining information
- Consensus parameters
- RPC endpoints
- Supply cap enforcement

**Requirements:** Running DRACHMA node

---

## Node Operation Scripts

### start-node.sh
**Purpose:** Start DRACHMA node with proper configuration

**Usage:**
```bash
./scripts/start-node.sh
```

### sync-check.sh / sync_verify.sh / verify-sync.sh
**Purpose:** Monitor blockchain sync status

**Usage:**
```bash
./scripts/sync-check.sh
```

---

## Testing Scripts

For comprehensive testing procedures, see:
- `docs/TESTING-PROCEDURES.md` - Master testing guide
- `docs/RPC-TESTING.md` - RPC endpoint testing
- `docs/WALLET-RECOVERY-TESTING.md` - Wallet recovery tests

---

## Installation Scripts

Platform-specific installation:
- `install-linux.sh` - Linux installation
- `install-macos.sh` - macOS installation
- `install-windows.ps1` - Windows installation (PowerShell)

---

## Monitoring Scripts

### node_monitor.py
**Purpose:** Monitor node health and statistics

**Usage:**
```bash
./scripts/node_monitor.py
```

---

## Build Scripts

### build.sh
**Purpose:** Build project with default settings

**Usage:**
```bash
./scripts/build.sh
```

### test.sh
**Purpose:** Run test suite

**Usage:**
```bash
./scripts/test.sh
```

---

## Pre-Launch Checklist

Before mainnet launch, run these scripts:

1. **Build verification:**
   ```bash
   ./scripts/verify-build-test.sh
   ```

2. **Genesis verification:**
   ```bash
   ./scripts/verify-genesis.sh
   ```

3. **Extended validation (requires running node):**
   ```bash
   drachma-node --testnet &
   sleep 30
   python3 scripts/extended-validation.py --testnet
   ```

All scripts should pass before proceeding with mainnet deployment.

---

## Script Development

**Adding new scripts:**
1. Follow existing naming conventions
2. Add usage documentation in this README
3. Make scripts executable: `chmod +x script.sh`
4. Test on all supported platforms
5. Add error handling and helpful output

**Best practices:**
- Use `set -e` in bash scripts for error handling
- Provide clear error messages
- Document all parameters
- Include usage examples

---

## Getting Help

**Script issues?**
1. Check script's built-in help (if available)
2. Review relevant documentation in `docs/`
3. Search GitHub Issues
4. Report bugs with:
   - Script name and version
   - Command used
   - Error output
   - Environment details

---

## Related Documentation

- `docs/TESTING-PROCEDURES.md` - Comprehensive testing guide
- `docs/BUILD-PROCESS.md` - Build documentation
- `docs/TROUBLESHOOTING.md` - Common issues
- `ANALYSIS-EXECUTIVE-SUMMARY.md` - Launch readiness
- `docs/LAUNCH-ACTION-ITEMS.md` - Pre-launch tasks

---

**Last Updated:** January 6, 2026
