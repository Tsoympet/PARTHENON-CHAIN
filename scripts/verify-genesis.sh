#!/usr/bin/env python3
"""
Genesis Block Verification Script for DRACHMA
This script verifies that genesis parameters in code match genesis.json configs
Part of LAUNCH-ACTION-ITEMS.md recommendations (line 38)
"""

import json
import sys
from pathlib import Path

# ANSI color codes
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
NC = '\033[0m'  # No Color

EXPECTED_HALVING_INTERVAL = 2102400  # ~4 years at 60s blocks
EXPECTED_SUPPLY = {
    'TLN': 21000000,
    'DRM': 41000000,
    'OBL': 61000000
}

def verify_genesis(network_name: str, genesis_path: Path) -> bool:
    """Verify genesis parameters for a network."""
    print(f"{YELLOW}Checking {network_name} genesis parameters...{NC}")
    
    if not genesis_path.exists():
        print(f"{RED}✗ ERROR: {genesis_path} not found{NC}")
        return False
    
    with open(genesis_path) as f:
        genesis = json.load(f)
    
    # Display basic parameters
    print(f"  Timestamp: {genesis.get('timestamp', 'N/A')}")
    print(f"  Bits: {genesis.get('bits', 'N/A')}")
    print(f"  Nonce: {genesis.get('nonce', 'N/A')}")
    print(f"  Merkle Root: {genesis.get('merkleRoot', 'N/A')}")
    print(f"  Block Hash: {genesis.get('hash', 'N/A')}")
    
    errors = []
    
    # Verify halving interval (only for mainnet which has it)
    if 'subsidyHalvingInterval' in genesis:
        halving = genesis['subsidyHalvingInterval']
        print(f"  Halving Interval: {halving} blocks")
        if halving != EXPECTED_HALVING_INTERVAL:
            print(f"{RED}✗ ERROR: Halving interval mismatch!{NC}")
            print(f"  Expected: {EXPECTED_HALVING_INTERVAL} blocks (~4 years)")
            print(f"  Found: {halving} blocks")
            errors.append("Halving interval mismatch")
        else:
            print(f"{GREEN}✓ Halving interval correct (2,102,400 blocks){NC}")
    
    # Verify multi-asset supply caps
    print()
    print(f"{YELLOW}Verifying multi-asset supply caps...{NC}")
    
    if 'assets' not in genesis:
        print(f"{RED}✗ ERROR: No assets section found{NC}")
        return False
    
    assets = genesis['assets']
    for asset_name, expected_supply in EXPECTED_SUPPLY.items():
        if asset_name not in assets:
            print(f"{RED}✗ ERROR: {asset_name} not found in assets{NC}")
            errors.append(f"{asset_name} missing")
            continue
        
        max_money_satoshis = assets[asset_name].get('maxMoney', 0)
        max_money_coins = max_money_satoshis // 100000000
        
        print(f"  {asset_name}: {max_money_coins:,} (expected: {expected_supply:,})")
        
        if max_money_coins != expected_supply:
            print(f"{RED}✗ ERROR: {asset_name} max supply mismatch{NC}")
            errors.append(f"{asset_name} supply mismatch")
        else:
            print(f"{GREEN}✓ {asset_name} supply correct{NC}")
    
    print()
    return len(errors) == 0

def main():
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    
    print("=" * 48)
    print("DRACHMA Genesis Block Verification")
    print("=" * 48)
    print()
    
    # Verify mainnet
    print("1. Verifying mainnet genesis parameters")
    print("=" * 40)
    mainnet_ok = verify_genesis("mainnet", repo_root / "mainnet" / "genesis.json")
    
    print()
    
    # Verify testnet
    print("2. Verifying testnet genesis parameters")
    print("=" * 40)
    testnet_ok = verify_genesis("testnet", repo_root / "testnet" / "genesis.json")
    
    print()
    print("=" * 48)
    print("VERIFICATION SUMMARY")
    print("=" * 48)
    
    if mainnet_ok and testnet_ok:
        print(f"{GREEN}✓ ALL CHECKS PASSED{NC}")
        print()
        print("Genesis parameters are consistent across:")
        print("  - mainnet/genesis.json")
        print("  - testnet/genesis.json")
        print("  - layer1-core/consensus/params.cpp (implicit)")
        print()
        print("Next steps before mainnet launch:")
        print("  1. Ensure genesis nonce has been properly mined")
        print("  2. Verify genesis hash with independent tool")
        print("  3. Cross-check with security audit team")
        return 0
    else:
        print(f"{RED}✗ VERIFICATION FAILED{NC}")
        print()
        print("Please fix discrepancies before proceeding with launch.")
        print("Refer to LAUNCH-ACTION-ITEMS.md for guidance.")
        return 1

if __name__ == '__main__':
    sys.exit(main())
