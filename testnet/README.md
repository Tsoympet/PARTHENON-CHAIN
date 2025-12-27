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

## Seeds and monitoring

- `seeds.json` lists bootstrap peers for default discovery.
- `monitoring/` contains Prometheus and Grafana starter configs to watch block
  height, peers, and mempool utilization across nodes and faucet services.
