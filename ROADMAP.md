# DRACHMA Project Roadmap

A phased plan for delivering the DRACHMA blockchain with transparency and predictable milestones.

## Status Overview

- [x] Implement Layer 1 consensus, UTXO, and PoW with difficulty retargeting
- [x] Harden fork resolution (checkpoints optional), timestamp bounds, and validation rate limits
- [x] Stand up public **testnet** with seeds, faucet, explorer, and Qt wallet builds
- [x] Layer 2 services: P2P, mempool, RPC surface, and wallet backend
- [x] Reference miners with Stratum support and benchmarking
- [ ] External audits (consensus, networking, wallet, miners)
- [ ] Mainnet parameter freeze and launch ceremony
- [ ] Long-term monitoring/observability package for operators

## 2025 Q3 — Networking & Wallet Polish
- Harden P2P validation, DoS protections, and peer diversity heuristics.
- Ship wallet UX refinements (hardware wallet flows, watch-only support, backup UX).
- Improve mempool fee estimation and rebroadcast policies.

## 2025 Q4 — Mining, Tooling, and Ops
- Optimize CPU/GPU miners with deterministic build scripts and published benchmarks.
- Publish Prometheus/Grafana dashboards and alert runbooks for node operators.
- Finalize container/systemd deployment profiles for reproducible mainnet installs.

## 2026 Q1 — Testnet v2
- Launch public testnet v2 with updated seeds, explorer endpoints, and faucet rate limits.
- Exercise activation/reorg rehearsals, difficulty clamp tuning, and latency/bandwidth metrics collection.
- Freeze candidate consensus parameters for audits.

## 2026 Q2 — Security Review
- Commission third-party audits across consensus, networking, wallet, and miner surfaces.
- Run fuzzing/adversarial campaigns; publish threat-model deltas and remediation timelines.
- Post public issue tracker for findings and patch status.

## 2026 Q3 — Release Candidates
- Produce signed RC binaries and SBOMs; verify deterministic builds across maintainers.
- Document rollback/activation playbooks and backup/restoration drills.
- Run community dry-runs of mainnet deployment scripts.

## 2026 Q4 — Mainnet Launch
- Execute fair-launch mainnet with open seed lists and monitored explorers.
- Provide 24/7 observability dashboards and incident contacts during launch window.
- Transition to long-term maintenance cadence and public release notes.

## Post-Launch (Ongoing)
- Maintain a regular documentation cadence and responsible disclosure workflow.
- Track operational metrics (orphan rate, mempool pressure, fee market health) and propose non-consensus mitigations as needed.
- Encourage ecosystem clients/wallets to reuse the reference implementation’s validation logic to avoid consensus drift.
