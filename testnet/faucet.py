#!/usr/bin/env python3
"""
Simple testnet faucet script. Requires a DRACHMA node with JSON-RPC enabled and
wallet support. The script enforces a per-address rate limit (in-memory) and
refuses to send if the node reports low balance.

Operational notes
-----------------
- The faucet connects directly to the node RPC interface; no additional SDKs
  are required.
- Use the `--state-file` flag so that rate limiting survives container restarts.
- Enable `--dry-run` in probes and CI to validate connectivity without sending
  coins.
- When exposing the faucet publicly, pair it with a reverse proxy that enforces
  TLS and request throttling.
"""
import argparse
import json
import os
import sys
import time
from base64 import b64encode
from http.client import HTTPConnection, HTTPSConnection
from pathlib import Path
from typing import Optional, Set
from urllib.parse import urlparse

RATE_LIMIT_SECONDS = 3600
MAX_PAYOUT = 5.0

class Faucet:
    def __init__(self, rpc_url: str, rpc_user: str, rpc_password: str, account: str, state_file: Optional[str], allowlist: Optional[Set[str]]):
        self.endpoint = urlparse(rpc_url)
        self.auth = b64encode(f"{rpc_user}:{rpc_password}".encode()).decode()
        self.account = account
        self.last_sent = {}
        self.state_path = Path(state_file) if state_file else None
        self.allowlist = allowlist
        self._load_state()

    def _connection(self):
        if self.endpoint.scheme == "https":
            return HTTPSConnection(self.endpoint.hostname, self.endpoint.port)
        return HTTPConnection(self.endpoint.hostname, self.endpoint.port)

    def _call(self, method: str, params):
        payload = json.dumps({"jsonrpc": "1.0", "id": "faucet", "method": method, "params": params})
        headers = {"Content-Type": "application/json", "Authorization": f"Basic {self.auth}"}
        conn = self._connection()
        conn.request("POST", self.endpoint.path or "/", payload, headers)
        resp = conn.getresponse()
        data = resp.read()
        if resp.status != 200:
            raise RuntimeError(f"RPC call failed: {resp.status} {resp.reason} {data}")
        obj = json.loads(data)
        if obj.get("error"):
            raise RuntimeError(f"RPC error: {obj['error']}")
        return obj["result"]

    def _load_state(self):
        if not self.state_path:
            return
        if self.state_path.exists():
            try:
                self.last_sent = json.loads(self.state_path.read_text())
            except Exception:  # noqa: BLE001
                self.last_sent = {}

    def _persist_state(self):
        if self.state_path:
            self.state_path.parent.mkdir(parents=True, exist_ok=True)
            self.state_path.write_text(json.dumps(self.last_sent))

    def _check_rate_limit(self, address: str):
        if self.allowlist is not None and address not in self.allowlist:
            raise RuntimeError("Address not in allowlist for public faucet")

        now = time.time()
        last = self.last_sent.get(address, 0)
        if now - last < RATE_LIMIT_SECONDS:
            raise RuntimeError("Rate limit exceeded; try again later")
        self.last_sent[address] = now
        self._persist_state()

    def _balance(self):
        return float(self._call("getbalance", [self.account]))

    def info(self):
        return {
            "balance": self._balance(),
            "network": self._call("getblockchaininfo", []),
        }

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
    parser.add_argument("--rpc", default=os.environ.get("RPC_URL", "http://127.0.0.1:18332"), help="RPC endpoint URL")
    parser.add_argument("--rpcuser", default=os.environ.get("RPC_USER", "user"))
    parser.add_argument("--rpcpassword", default=os.environ.get("RPC_PASSWORD", "pass"))
    parser.add_argument("--account", default="faucet", help="Wallet account to debit")
    parser.add_argument("--state-file", help="Persist rate-limit state to this path")
    parser.add_argument("--allowlist", help="File containing one address per line to receive payouts")
    parser.add_argument("--max-payout", type=float, default=MAX_PAYOUT, help="Override maximum payout per request")
    parser.add_argument("--rate-limit", type=int, default=RATE_LIMIT_SECONDS, help="Override rate-limit window in seconds")
    parser.add_argument("--dry-run", action="store_true", help="Perform connectivity checks without sending funds")
    args = parser.parse_args()

    global MAX_PAYOUT, RATE_LIMIT_SECONDS  # noqa: PLW0603
    MAX_PAYOUT = args.max_payout
    RATE_LIMIT_SECONDS = args.rate_limit

    allowlist = None
    if args.allowlist:
        allowlist = {line.strip() for line in Path(args.allowlist).read_text().splitlines() if line.strip()}

    faucet = Faucet(args.rpc, args.rpcuser, args.rpcpassword, args.account, args.state_file, allowlist)
    try:
        if args.dry_run:
            info = faucet.info()
            print(json.dumps(info, indent=2))
            return

        txid = faucet.send(args.address, args.amount)
        print(f"Sent {args.amount} DRACHMA to {args.address}: {txid}")
    except Exception as exc:  # noqa: BLE001
        print(f"Error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
