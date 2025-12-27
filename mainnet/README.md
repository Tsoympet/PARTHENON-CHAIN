# DRACHMA Mainnet

This directory codifies the launch-ready parameters and operational checklists for the
DRACHMA main network. Values here mirror the consensus-critical settings embedded in
Layer-1 and Layer-2 binaries so that deployments are reproducible and auditable.

## Files
- `genesis.json`: Canonical genesis block header fields and embedded message.
- `checkpoints.json`: Height-indexed block hashes for fast sync anchors.
- `seeds.json`: Curated peer seed endpoints for bootstrapping new nodes.
- `config.sample.conf`: Baseline configuration tuned for public nodes and validators.
- `launch-notes.md`: Runbook for first blocks, checkpoint rotation, and incident response.

## Usage
1. Copy `config.sample.conf` to your data directory as `drachma.conf` and adjust RPC
   credentials, external IPs, and pruning settings to match your infrastructure.
2. Validate the genesis hash and difficulty bits against `genesis.json` before starting
   any node. If compiled binaries disagree with these values, halt and investigate.
3. Distribute the seed list across multiple operators; do not rely on a single entity
   for initial peer discovery.
4. For production, pair node binaries with reproducible builds and signed releases
   from `.github/workflows/release.yml`.

## Operational tips
- Set conservative `maxconnections` limits initially and scale after observing stable
  mempool relay and block propagation.
- Keep `checkpoints.json` in sync with public releases to prevent long-range fork
  attacks and to accelerate new node bootstrap.
- Enable metrics exporters where possible to feed Prometheus/Grafana monitoring.
