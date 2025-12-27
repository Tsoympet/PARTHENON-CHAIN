# DRACHMA Roadmap (Draft)

This roadmap outlines milestones for stabilizing the DRACHMA reference implementation. Timelines depend on community review and security results. Target windows are estimates and will be updated after audits.

## Phase 1 — Foundations (Complete)
- ✅ Repository layout, licensing, CI skeletons
- ✅ Deterministic builds and reproducible binaries
- ✅ Core Layer 1 validation pipeline (blocks, transactions, difficulty)

## Phase 2 — Networking & Wallets (In Progress)
- 🔄 Harden P2P message validation, DoS protections, and peer rotation
- 🔄 Implement mempool fee estimation, eviction, and relay policy
- 🔄 Deliver wallet backend with address/key management, transaction construction, and hardware wallet compatibility

**Target:** Q3 2025 • **Progress:** 70%

## Phase 3 — Mining & Tooling
- 🔄 Optimize reference CPU/GPU miners with reproducible build scripts
- 🔄 Add monitoring/metrics endpoints for nodes and miners
- 🔄 Provide deployment scripts (systemd, containers)

**Target:** Q4 2025 • **Progress:** 55%

## Phase 4 — Testnet v2
- Launch public testnet v2 with seed nodes and block explorer endpoints
- Enable crash reporting and telemetry opt-in (if any) with privacy safeguards
- Capture performance metrics, adjust difficulty clamps, and rehearse activation workflows

**Target:** Q1 2026 • **Progress:** 0% (pending testnet v1 feedback)

## Phase 5 — Security Review
- Commission third-party audits of consensus, networking, wallet, and miner implementations
- Run adversarial test suites, fuzzing campaigns, and red-team exercises
- Publish threat models, remediation plans, and update `audit-guide.md` with findings

**Target:** Q2 2026 • **Progress:** 10%

## Phase 6 — Release Candidates
- Produce signed, deterministic binaries for major platforms
- Document upgrade procedures, rollback plans, and disaster-recovery drills
- Finalize activation parameters for any consensus changes and publish reproducible build instructions

**Target:** Q3 2026 • **Progress:** 0%

## Phase 7 — Mainnet
- Execute fair mainnet launch with open-source tooling and public seed lists
- Monitor chain stability with automated alerts; apply hotfixes only if consensus-safe
- Plan long-term maintenance, documentation cadence, and governance-free sustainability

**Target:** Q4 2026 • **Progress:** 0%

Progress updates and deviations from this plan will be tracked in issues and release notes.
