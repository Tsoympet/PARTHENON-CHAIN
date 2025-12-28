# Security Audit Guide

## Introduction
DRACHMA follows a conservative, minimal-trust philosophy inspired by Bitcoin Core practices. Consensus and cryptography are intentionally simple, the launch was fair with no premine or privileged keys, and every layer is designed to be auditable. We invite professional reviewers to challenge assumptions, reproduce validation from first principles, and keep user funds and chain integrity safe.

## Scope
Audit efforts should prioritize consensus-critical and security-sensitive surfaces:

- **Layer 1 (Consensus):** Block/transaction validation, difficulty/retargeting, PoW hashing, block formats, serialization, database persistence. Key paths: `layer1-core/`, `common/`, `tests/` (consensus vectors), `docs/technical-spec.md`.
- **Cryptography:** Schnorr over secp256k1, tagged hashing, signature verification, RNG seeding. Key paths: `common/crypto/`, `layer1-core/crypto/` (if present), and fuzz targets.
- **Networking / P2P:** Message parsing, DoS controls, peer selection, bandwidth/memory bounds, handshake and version negotiation. Key paths: `layer2-services/`, `common/net/`, `scripts/` for configs.
- **Wallets:** Key generation, encryption at rest, signing flows, backup/restore, UI protections against accidental leaks. Key paths: `layer3-app/`, `docs/gui-user-guide.md`, `docs/security.md`.
- **Miners:** Stratum parsing, target/difficulty handling, nonce search, pool authentication. Key paths: `miners/`, `scripts/`.
- **Build/Packaging:** Reproducibility, dependency pinning, CI workflows. Key paths: `Dockerfile`, `docker-compose.yml`, `.github/workflows/`.

## Known Limitations / Disclosures
- Testnet difficulty clamps are still experimental and subject to tuning before mainnet.
- Hardware wallet integrations and mnemonic export flows remain **beta**; review entropy sources and prompts.
- GPU miner OpenCL paths may diverge across vendors; CUDA is the reference baseline.
- Monitoring/alert thresholds target testnet scale; mainnet hardening is ongoing.
- Some tooling scripts in `scripts/` are prototypes and lack defensive input validation.

## How to Audit
- **Fuzzing setup:** Configure with `-DDRACHMA_BUILD_FUZZ=ON -DSANITIZE=address,undefined` and build with Clang. Fuzz targets live under `tests/fuzz/` (e.g., `fuzz_tx`, `fuzz_block`, `fuzz_schnorr`). Run with libFuzzer: `./build/tests/fuzz/fuzz_tx -max_total_time=600 -artifact_prefix=artifacts/`.
- **Regtest mode:** Start two nodes in regtest for deterministic scenarios:
  ```bash
  ./build/layer1-core/drachmad --network regtest --listen --rpcuser=user --rpcpassword=pass --txindex
  ./build/layer1-core/drachmad --network regtest --connect=127.0.0.1:9333 --rpcuser=user --rpcpassword=pass
  ```
  Use RPC `generatetoaddress` to craft blocks and simulate reorgs. See `tests/` for templates.
- **Testnet deployment:** Use `testnet/seeds.json` and `docker-compose.yml` for a multi-node testnet harness. Metrics exporters are configured for resource tracking.
- **Code coverage:** Enable `-DENABLE_COVERAGE=ON` with GCC/Clang, run `ctest --output-on-failure`, then generate reports via `lcov --capture --directory build --output-file coverage.info` and `genhtml coverage.info --output-directory coverage-html`. Attach coverage deltas to findings.
- **Sanitizers:** ASan/UBSan/TSan builds are supported; reproduce crashes with minimized seeds when reporting.

## Reporting
Report privately before any public disclosure:

- **Email:** security@drachma.org (PGP on request).
- **GitHub Security Advisory:** Use the "Report a vulnerability" option to open a private advisory with maintainers.
- **Encrypted channel:** Provide your key/fingerprint to arrange encrypted exchange.

Include impact, reproducible steps or seeds, affected commits/tags, environment, and suggested embargo timelines. Expect acknowledgment within 48 hours.

## Bounty Program
Bug bounty program is **planned** (Immunefi/HackerOne) and will publish scope, rewards, and SLA once mainnet RC is tagged. Early responsible disclosures may be retroactively rewarded at maintainer discretion.

## Past/Future Audits
- **Planned:** External audit window targeting **Q1 2026** (consensus, networking, wallet). Pre-audit internal review and fuzzing hardening underway.
- **Historical:** No third-party audits completed yet; this guide will be updated as engagements conclude.
