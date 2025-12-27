# Mainnet Launch Notes

These steps guard the first epoch of DRACHMA mainnet and outline operational
responses for early incidents.

## Pre-flight
- Confirm genesis fields hash to `00000000drachmagenesis...` as recorded in
  `genesis.json`.
- Ensure at least three independent seed operators are online and reachable.
- Publish signed binaries built from CI (see `.github/workflows/release.yml`).
- Coordinate with auditors to monitor logs during the first 100 blocks.

## Launch window
1. Start at least two foundation nodes with identical `config.sample.conf` except for
   distinct `externalip` fields.
2. Broadcast a low-fee, single-output transaction to test mempool propagation before
   mining the first block.
3. Begin mining with a conservative hashrate target to allow late-joining peers to
   synchronize.

## Early checkpoints
- Refresh `checkpoints.json` at block 1, 10, 25, and 100 once hashes stabilize.
- Publish signed checkpoint files and attach them to release assets.

## Incident response
- If a chain split is detected, pause mining and gather competing headers. Prefer the
  chain with the highest cumulative work and widest peer distribution.
- For DoS against P2P discovery, rotate seeds and publish updated `seeds.json`.
- For vulnerabilities, follow the Responsible Disclosure process in `SECURITY.md`.
