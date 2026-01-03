# Audit Preparation Checklist

This checklist is designed to help reviewers prepare for a comprehensive assessment of the DRACHMA protocol and implementation. It targets consensus-critical paths, cryptographic assumptions, networking exposures, wallet handling, and upstream dependencies. Each item includes expected evidence so audit reports can be reproduced.

## Consensus Critical Paths
- **Genesis and chain parameters:** Verify mainnet/testnet/regtest parameters (genesis block hash, premine rules, subsidy schedule, difficulty retargeting, checkpoints) and document any differences from the whitepaper.
- **Block validation pipeline:** Trace `CheckBlock`, `ContextualCheckBlock`, and block index activation logic for invariant enforcement; confirm per-rule test coverage and fuzz harnesses for parsing/serialization.
- **Transaction validation:** Review script verification (including Schnorr/taproot paths), standardness flags, fee/rate limits, and mempool admission checks; collect coverage reports demonstrating exercised rule branches.
- **Reorg handling:** Examine chain selection, fork resolution, and finality/anti-eclipse defenses; capture integration tests that simulate deep reorgs and partition healing.
- **Mempool and relay policies:** Document DoS protections (fee filters, orphan handling, ban scores, bloom filters) and their configurable limits.

## Cryptography Assumptions
- **Primitives:** Confirm reliance on secp256k1 and SHA-256d; list library versions, build options (endian, asm, batch verification), and any non-default flags.
- **Key management:** Review HD wallet derivation, seed encryption, and hardware wallet flows. Note where keys touch disk or memory and the lifetime of in-memory secrets.
- **Randomness:** Inventory RNG sources for key generation, nonce derivation, and networking tokens; verify blocking/non-blocking behavior and entropy initialization order.
- **Constant-time expectations:** Identify code paths that must avoid timing side channels (signature checks, MACs); capture tests or linters that enforce constant-time implementations.

## Networking Attack Vectors
- **P2P surface:** Enumerate message handlers, admission controls, connection slot limits, tor/i2p integration, and peer diversity rules. Map each mitigation to specific configuration flags.
- **DoS and resource caps:** Record limits for bandwidth, inventory tracking, bloom filters, and invalid message banning thresholds; include stress-test results showing stability under load.
- **Eclipse and partition risks:** Document outbound peer selection, anti-eclipse heuristics, and bootstrapping seeds; provide metrics for diverse peer sets during IBD.

## Wallet Private Key Handling
- **Storage and encryption:** Describe keystore formats, encryption algorithms, unlock/lock flows, and any auto-timeout behaviors.
- **RPC and UI exposure:** List wallet-affecting RPCs, authentication/authorization mechanisms, and transport security requirements.
- **Backup and recovery:** Capture seed backup recommendations, derivation paths, and restore procedures validated by integration tests.

## Dependencies and SBOM
- **Dependency inventory:** Generate a Software Bill of Materials (SBOM) using CycloneDX (e.g., `cmake --build build --target sbom` or `cyclonedx-bom -o sbom.json`). Include both source and container/runtime dependencies.
- **Known vulnerabilities:** Cross-reference SBOM components with CVE feeds (e.g., osv.dev, NVD). Document any ignored advisories with rationale and compensating controls.
- **Reproducible builds:** Note deterministic build steps, pinned versions, and hashing/signing procedures for release artifacts.

## Fuzzing and Dynamic Analysis
- **Harnesses:** List fuzz targets (block parsing, tx validation, P2P message deserialization, Schnorr batch verification). Provide build flags for libFuzzer with ASan/UBSan.
- **Coverage goals:** Attach the latest coverage reports (>90% target) and note unexercised code regions with remediation plans.
- **Test reuse:** Extend existing layer1 validation, layer2 RPC, and sidechain NFT fixtures instead of creating new harnesses; prefer the regtest harness, StateStore-backed WASM RPC tests, and RPC server integration tests to keep invariants consistent.
- **Crash triage:** Describe how crashes are reproduced, minimized, and tracked (e.g., `llvm-symbolizer`, `clusterfuzzlite`). Record any outstanding issues before release.

## Reporting Package
- **Artifacts for auditors:** Supply recent binaries, test logs, coverage artifacts, SBOM outputs, and configuration used for fuzzing/integration tests.
- **Points of contact:** Provide the security mailing list and PGP fingerprints for maintainers.

Keep this document updated before each release candidate so external auditors can verify assumptions quickly.
