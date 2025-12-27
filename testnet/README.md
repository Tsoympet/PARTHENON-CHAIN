# DRACHMA Testnet

This directory documents the parameters for the public DRACHMA test network and
provides helper utilities for genesis validation, faucets, and monitoring. The
testnet mirrors mainnet consensus rules with these key variations:

- Minimum-difficulty blocks are permitted when the network stalls.
- Default ports are offset from mainnet in higher-level services.
- Genesis message: "DRACHMA TESTNET".

## Genesis tools

Compile `genesis.cpp` alongside the Layer-1 sources (requires the consensus and
crypto dependencies). Running the resulting binary prints the Merkle root,
target bits, and mined nonce for the test network. The program is deterministic
and will refuse to emit a block that violates the configured proof-of-work
threshold.

```
mkdir -p build && cd build
cmake .. && cmake --build .
./testnet_genesis
```

## Faucet

`faucet.py` is a lightweight JSON-RPC client that debits a funding wallet and
sends capped payouts to requester addresses with a per-address rate limit.
Example usage:

```
python3 faucet.py tb1qtestaddress --amount 1.5 --rpc http://127.0.0.1:18332 \
  --rpcuser user --rpcpassword pass
```

For public usage, prefer running behind a reverse proxy with TLS termination
and set `--state-file` to persist rate limits across restarts. The faucet script
accepts `--allowlist` to restrict payouts to pre-approved addresses.

## Seeds and monitoring

- `seeds.json` lists geographically diverse DNS seeds and IPv4/IPv6 anchors used
  by bootstrap discovery. New operators should contribute additional DNS seeds
  to expand coverage.
- `monitoring/` contains Prometheus and Grafana starter configs to watch block
  height, peers, and mempool utilization across nodes and faucet services. See
  the README inside that directory for scrape targets matching the provided
  docker-compose profile.

## Dashboard

`dashboard.html` is a static status page that polls JSON-RPC endpoints for
blockchain height, mempool size, and peer counts. Serve it with any static web
server (the default docker-compose stack mounts it behind nginx on port 8080)
and set `RPC_URL`, `RPC_USER`, and `RPC_PASSWORD` via `window` variables when
embedding in another site.

## Quick start (3-node local mesh)

Use `docker-compose up -d` from the repository root to launch two seed nodes,
three regular peers, monitoring, and an optional faucet. Logs stream with
`docker-compose logs -f drachma-seed-a`, and peer health can be checked with
`scripts/sync-check.sh --network testnet`.
