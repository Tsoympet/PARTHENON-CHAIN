# DRACHMA Testnet

This directory documents the parameters for the public DRACHMA test network
and provides a small helper utility to regenerate the genesis block to ensure
transparency. The testnet mirrors mainnet consensus rules with these key
variations:

- Minimum-difficulty blocks are permitted when the network stalls.
- Default ports are offset from mainnet in higher-level services.
- Genesis message: "DRACHMA TESTNET".

## Regenerating the testnet genesis block

Compile `genesis.cpp` alongside the Layer-1 sources (requires the consensus
and crypto dependencies). Running the resulting binary prints the Merkle root,
target bits, and mined nonce for the test network. The program is deterministic
and will refuse to emit a block that violates the configured proof-of-work
threshold.

```
mkdir -p build && cd build
cmake .. && cmake --build .
./testnet_genesis
```

## Expected outputs

The helper prints the block header fields and the computed hash. These values
must match any hardcoded testnet parameters in deployments; differences indicate
an invalid genesis definition and must be investigated before enabling the
network.
