#!/usr/bin/env python3
"""
Lightweight RPC monitor for DRACHMA nodes. Polls one or more JSON-RPC endpoints
and prints block height, mempool usage, and peer counts. Designed for use in
cron, Kubernetes probes, or docker-compose health sidecars.
"""

import argparse
import json
import sys
import time
from base64 import b64encode
from http.client import HTTPConnection, HTTPSConnection
from typing import Dict, List
from urllib.parse import urlparse


class RPCClient:
    def __init__(self, url: str, user: str, password: str):
        self.endpoint = urlparse(url)
        self.auth = b64encode(f"{user}:{password}".encode()).decode()
        self._conn = None

    def _get_connection(self):
        """Reuse connection for multiple requests to improve performance"""
        if self._conn is None:
            if self.endpoint.scheme == "https":
                self._conn = HTTPSConnection(self.endpoint.hostname, self.endpoint.port or 443, timeout=5)
            else:
                self._conn = HTTPConnection(self.endpoint.hostname, self.endpoint.port or 80, timeout=5)
        return self._conn

    def call(self, method: str, params: List):
        payload = json.dumps({"jsonrpc": "1.0", "id": "monitor", "method": method, "params": params})
        headers = {"Content-Type": "application/json", "Authorization": f"Basic {self.auth}"}
        
        conn = self._get_connection()
        try:
            conn.request("POST", self.endpoint.path or "/", payload, headers)
            resp = conn.getresponse()
            data = resp.read()
            if resp.status != 200:
                raise RuntimeError(f"RPC call failed: {resp.status} {resp.reason}")
            doc = json.loads(data)
            if doc.get("error"):
                raise RuntimeError(f"RPC error: {doc['error']}")
            return doc["result"]
        except Exception:
            # On error, close and reset connection for next attempt
            if self._conn:
                try:
                    self._conn.close()
                except Exception:
                    pass
                self._conn = None
            raise


def snapshot(client: RPCClient) -> Dict:
    info = client.call("getblockchaininfo", [])
    mempool = client.call("getmempoolinfo", [])
    peerinfo = client.call("getpeerinfo", [])
    return {
        "height": info.get("blocks"),
        "headers": info.get("headers"),
        "difficulty": info.get("difficulty"),
        "peers": len(peerinfo),
        "mempool_size": mempool.get("size"),
        "mempool_bytes": mempool.get("bytes"),
        "chain": info.get("chain"),
    }


def monitor(nodes: List[str], user: str, password: str, interval: int):
    clients = [(node, RPCClient(node, user, password)) for node in nodes]
    while True:
        rows = []
        for label, client in clients:
            try:
                row = snapshot(client)
                status = "ok"
            except Exception as exc:  # noqa: BLE001
                row = {"error": str(exc)}
                status = "error"
            rows.append((label, status, row))

        emit(rows)
        if interval == 0:
            break
        time.sleep(interval)


def emit(rows):
    print(time.strftime("[%Y-%m-%d %H:%M:%S]"))
    for label, status, row in rows:
        if status == "error":
            print(f"{label:25s} ERROR    {row['error']}")
            continue
        print(
            f"{label:25s} {row['chain']:<8s} height={row['height']:<8d} headers={row['headers']:<8d} "
            f"peers={row['peers']:<4d} mempool={row['mempool_size']:<4d} tx bytes={row['mempool_bytes']}"
        )
    sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(description="Monitor DRACHMA nodes via RPC")
    parser.add_argument("--nodes", required=True, help="Comma-separated RPC URLs")
    parser.add_argument("--user", default="user", help="RPC username")
    parser.add_argument("--password", default="pass", help="RPC password")
    parser.add_argument("--interval", type=int, default=0, help="Seconds between polls (0 for single run)")
    args = parser.parse_args()

    nodes = [n.strip() for n in args.nodes.split(",") if n.strip()]
    if not nodes:
        parser.error("at least one node must be provided")

    monitor(nodes, args.user, args.password, args.interval)


if __name__ == "__main__":
    main()
