#!/usr/bin/env python3
"""
Simple testnet faucet script. Requires a DRACHMA node with JSON-RPC enabled and
wallet support. The script enforces a per-address rate limit (in-memory) and
refuses to send if the node reports low balance.
"""
import argparse
import json
import sys
import time
from base64 import b64encode
from http.client import HTTPConnection
from urllib.parse import urlparse

RATE_LIMIT_SECONDS = 3600
MAX_PAYOUT = 5.0

class Faucet:
    def __init__(self, rpc_url: str, rpc_user: str, rpc_password: str, account: str):
        self.endpoint = urlparse(rpc_url)
        self.auth = b64encode(f"{rpc_user}:{rpc_password}".encode()).decode()
        self.account = account
        self.last_sent = {}

    def _call(self, method: str, params):
        payload = json.dumps({"jsonrpc": "1.0", "id": "faucet", "method": method, "params": params})
        headers = {"Content-Type": "application/json", "Authorization": f"Basic {self.auth}"}
        conn = HTTPConnection(self.endpoint.hostname, self.endpoint.port)
        conn.request("POST", self.endpoint.path or "/", payload, headers)
        resp = conn.getresponse()
        data = resp.read()
        if resp.status != 200:
            raise RuntimeError(f"RPC call failed: {resp.status} {resp.reason} {data}")
        obj = json.loads(data)
        if obj.get("error"):
            raise RuntimeError(f"RPC error: {obj['error']}")
        return obj["result"]

    def _check_rate_limit(self, address: str):
        now = time.time()
        last = self.last_sent.get(address, 0)
        if now - last < RATE_LIMIT_SECONDS:
            raise RuntimeError("Rate limit exceeded; try again later")
        self.last_sent[address] = now

    def _balance(self):
        return float(self._call("getbalance", [self.account]))

    def send(self, address: str, amount: float):
        if amount <= 0 or amount > MAX_PAYOUT:
            raise RuntimeError(f"Invalid amount: must be between 0 and {MAX_PAYOUT}")
        self._check_rate_limit(address)
        if self._balance() < amount * 2:
            raise RuntimeError("Faucet depleted; top up the funding wallet")
        return self._call("sendtoaddress", [address, amount, "testnet faucet", "", False, True, 1, "UNSET"])


def main():
    parser = argparse.ArgumentParser(description="DRACHMA testnet faucet")
    parser.add_argument("address", help="Destination testnet address")
    parser.add_argument("--amount", type=float, default=1.0, help="Amount to send (default 1.0)")
    parser.add_argument("--rpc", default="http://127.0.0.1:18332", help="RPC endpoint URL")
    parser.add_argument("--rpcuser", default="user")
    parser.add_argument("--rpcpassword", default="pass")
    parser.add_argument("--account", default="faucet", help="Wallet account to debit")
    args = parser.parse_args()

    faucet = Faucet(args.rpc, args.rpcuser, args.rpcpassword, args.account)
    try:
        txid = faucet.send(args.address, args.amount)
        print(f"Sent {args.amount} DRACHMA to {args.address}: {txid}")
    except Exception as exc:  # noqa: BLE001
        print(f"Error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
