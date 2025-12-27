# DRACHMA Audit Guide

This document outlines expectations, scope, and artifacts for independent audits ahead of mainnet launch.

## Objectives

- Validate consensus correctness (block/transaction validation, difficulty, fork choice).
- Assess P2P networking robustness (message bounds, DoS handling, peer selection).
- Review wallet key management, encryption, and signing flows.
- Evaluate miner safety (Stratum handling, nonce search, difficulty targeting).

## Scope

- **Layer 1:** Validation rules, chainstate storage, PoW and difficulty adjustment, block/tx formats, serialization.
- **Layer 2:** P2P networking, mempool policy, RPC authentication/authorization, transaction indexing.
- **Layer 3:** Wallet UI security surfaces (passphrase prompts, backup/restore), RPC error handling.
- **Miners:** CPU/GPU implementations, pool mode, Stratum parsing, anti-botnet defaults.
- **Build/CI:** Reproducibility, dependency pinning, supply-chain risk in third-party libraries.

## Artifacts for Auditors

- Source tag/commit hash and signed release artifacts.
- CMake cache/configuration used for builds (`CMakeCache.txt`).
- Test/fuzz coverage reports and failing seeds (if any).
- Protocol documents: [`whitepaper.md`](whitepaper.md), [`technical-spec.md`](technical-spec.md), [`security.md`](security.md), [`threat-model.md`](threat-model.md).
- Sample configs and deployment profiles (`deployment.md`, `building.md`).

## Recommended Methodology

1. **Static analysis:** Linters, sanitizers, and review of serialization/crypto boundaries.
2. **Differential testing:** Compare validation results against Bitcoin Core test vectors where applicable.
3. **Fuzzing:** Enable `-DDRACHMA_BUILD_FUZZ=ON` and run fuzz targets for validation, Schnorr, and tagged hashing with dedicated time budgets.
4. **Adversarial simulation:** Run multi-node networks with byzantine peers to probe reorg policies, timestamp handling, and mempool eviction.
5. **Operational review:** Inspect systemd units, Dockerfiles, and scripts for least-privilege and logging defaults.

## Reporting

- Deliver a signed report detailing findings by severity (Critical/High/Medium/Low/Informational).
- Include proof-of-concept exploits or minimal repro steps for each issue.
- Propose mitigations or code pointers where applicable.
- Share privately via the security contact in [`SECURITY.md`](../SECURITY.md) before public disclosure.

## Acceptance and Follow-Up

- Issues will be tracked in a public advisory once mitigated or acknowledged.
- Fixes require regression tests and, when consensus-affecting, a testnet activation rehearsal.
- Auditors may be asked to review patches for critical findings prior to mainnet launch.
