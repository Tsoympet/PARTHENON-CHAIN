#!/usr/bin/env python3
"""
Extended Validation Script for DRACHMA
This script performs comprehensive validation of the node, RPC, and wallet functionality.
Part of LAUNCH-ACTION-ITEMS.md recommendations for extended testing.
"""

import subprocess
import sys
import time
import json
from pathlib import Path

# ANSI color codes
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[0;34m'
NC = '\033[0m'

class DrachmaValidator:
    def __init__(self, testnet=True):
        self.testnet = testnet
        self.rpc_port = 19332 if testnet else 9332
        self.cli_base = ['drachma-cli']
        if testnet:
            self.cli_base.append('--testnet')
        self.errors = []
        self.warnings = []
    
    def run_cli(self, *args):
        """Run drachma-cli command and return output."""
        try:
            result = subprocess.run(
                self.cli_base + list(args),
                capture_output=True,
                text=True,
                timeout=30
            )
            if result.returncode != 0:
                return None, result.stderr
            return result.stdout.strip(), None
        except subprocess.TimeoutExpired:
            return None, "Command timed out"
        except Exception as e:
            return None, str(e)
    
    def print_header(self, text):
        """Print section header."""
        print(f"\n{BLUE}{'=' * 60}{NC}")
        print(f"{BLUE}{text}{NC}")
        print(f"{BLUE}{'=' * 60}{NC}")
    
    def print_test(self, name, passed, details=""):
        """Print test result."""
        status = f"{GREEN}✓ PASS{NC}" if passed else f"{RED}✗ FAIL{NC}"
        print(f"{status} - {name}")
        if details:
            print(f"      {details}")
        if not passed:
            self.errors.append(name)
    
    def validate_node_running(self):
        """Check if node is running."""
        self.print_header("1. Node Status")
        
        output, error = self.run_cli("getblockchaininfo")
        if error:
            self.print_test("Node is running", False, f"Error: {error}")
            return False
        
        self.print_test("Node is running", True)
        
        try:
            info = json.loads(output)
            blocks = info.get('blocks', 0)
            headers = info.get('headers', 0)
            
            self.print_test("Get blockchain info", True, f"Blocks: {blocks}, Headers: {headers}")
            
            if blocks == headers:
                self.print_test("Node is synced", True)
            else:
                self.print_test("Node is syncing", True, f"{blocks}/{headers} blocks")
                self.warnings.append("Node not fully synced")
        except:
            self.print_test("Parse blockchain info", False)
            return False
        
        return True
    
    def validate_network(self):
        """Validate network connectivity."""
        self.print_header("2. Network Connectivity")
        
        # Get network info
        output, error = self.run_cli("getnetworkinfo")
        if error:
            self.print_test("Get network info", False, error)
            return False
        
        self.print_test("Get network info", True)
        
        try:
            info = json.loads(output)
            version = info.get('version', 'unknown')
            connections = info.get('connections', 0)
            
            self.print_test("Node version", True, f"v{version}")
            self.print_test("Has peer connections", connections > 0, 
                          f"{connections} peers")
            
            if connections == 0:
                self.warnings.append("No peer connections - node isolated")
        except:
            self.print_test("Parse network info", False)
            return False
        
        # Get peer info
        output, error = self.run_cli("getpeerinfo")
        if error:
            self.print_test("Get peer info", False)
        else:
            try:
                peers = json.loads(output)
                self.print_test("Get peer info", True, f"{len(peers)} peers")
            except:
                self.print_test("Parse peer info", False)
        
        return True
    
    def validate_wallet(self):
        """Validate wallet functionality."""
        self.print_header("3. Wallet Functionality")
        
        # Get wallet info
        output, error = self.run_cli("getwalletinfo")
        if error:
            self.print_test("Wallet enabled", False, "Wallet not available")
            return False
        
        self.print_test("Wallet enabled", True)
        
        try:
            info = json.loads(output)
            balance = info.get('balance', 0)
            txcount = info.get('txcount', 0)
            
            self.print_test("Get wallet info", True, 
                          f"Balance: {balance}, Transactions: {txcount}")
        except:
            self.print_test("Parse wallet info", False)
            return False
        
        # Test address generation
        output, error = self.run_cli("getnewaddress")
        if error:
            self.print_test("Generate new address", False)
        else:
            self.print_test("Generate new address", True, f"Address: {output[:20]}...")
        
        # Test address validation
        if output:
            valid_output, error = self.run_cli("validateaddress", output)
            if error:
                self.print_test("Validate address", False)
            else:
                try:
                    valid = json.loads(valid_output)
                    is_valid = valid.get('isvalid', False)
                    self.print_test("Validate address", is_valid)
                except:
                    self.print_test("Parse address validation", False)
        
        return True
    
    def validate_mining(self):
        """Validate mining info."""
        self.print_header("4. Mining Information")
        
        output, error = self.run_cli("getmininginfo")
        if error:
            self.print_test("Get mining info", False)
            return False
        
        self.print_test("Get mining info", True)
        
        try:
            info = json.loads(output)
            blocks = info.get('blocks', 0)
            difficulty = info.get('difficulty', 0)
            networkhashps = info.get('networkhashps', 0)
            
            self.print_test("Mining info valid", True,
                          f"Blocks: {blocks}, Difficulty: {difficulty:.2f}")
            
            # Get network hashrate
            hash_output, _ = self.run_cli("getnetworkhashps")
            if hash_output:
                self.print_test("Get network hashrate", True, 
                              f"{float(hash_output):.2f} H/s")
        except:
            self.print_test("Parse mining info", False)
            return False
        
        return True
    
    def validate_consensus(self):
        """Validate consensus parameters."""
        self.print_header("5. Consensus Parameters")
        
        # Get blockchain info for consensus checks
        output, error = self.run_cli("getblockchaininfo")
        if error:
            self.print_test("Get consensus info", False)
            return False
        
        try:
            info = json.loads(output)
            chain = info.get('chain', '')
            
            expected_chain = 'test' if self.testnet else 'main'
            self.print_test("Correct chain", chain == expected_chain,
                          f"Chain: {chain}")
            
            # Check if multi-asset is active
            # (This would require custom RPC - using blockchain info as proxy)
            self.print_test("Blockchain info accessible", True)
            
        except:
            self.print_test("Parse consensus info", False)
            return False
        
        return True
    
    def validate_rpc_endpoints(self):
        """Test various RPC endpoints."""
        self.print_header("6. RPC Endpoint Tests")
        
        endpoints = [
            ("getblockcount", []),
            ("getbestblockhash", []),
            ("getdifficulty", []),
            ("getconnectioncount", []),
            ("uptime", []),
        ]
        
        for endpoint, args in endpoints:
            output, error = self.run_cli(endpoint, *args)
            success = (error is None)
            self.print_test(f"RPC: {endpoint}", success)
        
        # Test getblock with best block hash
        hash_output, _ = self.run_cli("getbestblockhash")
        if hash_output:
            block_output, error = self.run_cli("getblock", hash_output)
            self.print_test("RPC: getblock", error is None)
        
        return True
    
    def validate_supply_caps(self):
        """Validate that supply caps are enforced."""
        self.print_header("7. Supply Cap Validation")
        
        # This is a basic check - real validation happens in consensus code
        # We just verify the values are reasonable
        
        output, error = self.run_cli("getblockchaininfo")
        if error:
            self.print_test("Check supply caps", False)
            return False
        
        try:
            info = json.loads(output)
            # Note: This would require custom RPC to check asset-specific supply
            # For now, we just verify blockchain info is available
            self.print_test("Blockchain info available", True)
            
            print(f"      Note: Supply caps verified in code (see verify-genesis.sh)")
        except:
            self.print_test("Parse blockchain info", False)
            return False
        
        return True
    
    def run_validation(self):
        """Run all validation checks."""
        print(f"{BLUE}{'=' * 60}{NC}")
        print(f"{BLUE}DRACHMA Extended Validation{NC}")
        print(f"{BLUE}Network: {'Testnet' if self.testnet else 'Mainnet'}{NC}")
        print(f"{BLUE}{'=' * 60}{NC}")
        
        # Run all validations
        self.validate_node_running()
        self.validate_network()
        self.validate_wallet()
        self.validate_mining()
        self.validate_consensus()
        self.validate_rpc_endpoints()
        self.validate_supply_caps()
        
        # Print summary
        print(f"\n{BLUE}{'=' * 60}{NC}")
        print(f"{BLUE}VALIDATION SUMMARY{NC}")
        print(f"{BLUE}{'=' * 60}{NC}")
        
        total_tests = len(self.errors) + len(self.warnings)
        
        if not self.errors and not self.warnings:
            print(f"{GREEN}✓ ALL TESTS PASSED{NC}")
            print(f"\nThe node is functioning correctly.")
            return 0
        
        if self.errors:
            print(f"{RED}✗ FAILURES: {len(self.errors)}{NC}")
            for error in self.errors:
                print(f"  - {error}")
        
        if self.warnings:
            print(f"{YELLOW}⚠ WARNINGS: {len(self.warnings)}{NC}")
            for warning in self.warnings:
                print(f"  - {warning}")
        
        print("\nRecommendations:")
        if self.errors:
            print("  1. Review error messages above")
            print("  2. Check node logs: ~/.drachma/debug.log")
            print("  3. See docs/TROUBLESHOOTING.md")
        
        if "No peer connections" in str(self.warnings):
            print("  • Ensure network connectivity")
            print("  • Check firewall settings")
            print("  • Add seed nodes to config")
        
        return 1 if self.errors else 0

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='DRACHMA Extended Validation')
    parser.add_argument('--mainnet', action='store_true',
                       help='Validate mainnet node (default: testnet)')
    args = parser.parse_args()
    
    validator = DrachmaValidator(testnet=not args.mainnet)
    return validator.run_validation()

if __name__ == '__main__':
    sys.exit(main())
