# DRACHMA Blockchain (DRM)

[![CI](https://github.com/Tsoympet/BlockChainDrachma/actions/workflows/ci.yml/badge.svg)](https://github.com/Tsoympet/BlockChainDrachma/actions/workflows/ci.yml)
[![Release](https://github.com/Tsoympet/BlockChainDrachma/actions/workflows/release.yml/badge.svg)](https://github.com/Tsoympet/BlockChainDrachma/actions/workflows/release.yml)
[![codecov](https://codecov.io/gh/Tsoympet/BlockChainDrachma/graph/badge.svg?token=pbNZjIpSXO)](https://codecov.io/gh/Tsoympet/BlockChainDrachma)
[![Codecov Sunburst](https://codecov.io/gh/Tsoympet/BlockChainDrachma/graphs/sunburst.svg?token=pbNZjIpSXO)](https://codecov.io/gh/Tsoympet/BlockChainDrachma)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/Tsoympet/BlockChainDrachma?style=social)](https://github.com/Tsoympet/BlockChainDrachma/stargazers)
[![GitHub Forks](https://img.shields.io/github/forks/Tsoympet/BlockChainDrachma?style=social)](https://github.com/Tsoympet/BlockChainDrachma/network/members)
[![Open Issues](https://img.shields.io/github/issues/Tsoympet/BlockChainDrachma)](https://github.com/Tsoympet/BlockChainDrachma/issues)
[![Security Policy](https://img.shields.io/badge/Security-Policy-orange)](SECURITY.md)
[![Audit Status](https://img.shields.io/badge/Audit-In%20preparation-blueviolet)](docs/audit-guide.md)
[![Discussions](https://img.shields.io/badge/Discussions-join-blue.svg)](#community--support)
[![Roadmap](https://img.shields.io/badge/Roadmap-See%20plans-8A2BE2)](ROADMAP.md)

DRACHMA is a **Proof-of-Work monetary blockchain** designed for long-term stability, auditability, and minimal trust assumptions.

The project focuses on:
- deterministic consensus rules,
- conservative cryptographic design,
- strict separation of system layers,
- and transparent network launch conditions.

This repository contains the **reference implementation** of the DRACHMA network.

> **Status: Testnet Ready** — Public testnet nodes, faucets, and explorers are available. Mainnet launch will follow after public testing and audits.

## Releases

Official binaries and source archives will be published on the GitHub Releases page. Each release will be tagged (vX.Y.Z); the next recommended cut is **`v0.1.0-rc`** to align with the release workflow and explorer shipped in this repository update. Every release will include:
- Signed artifacts (release tag and archives signed with the maintainer key).
- SHA-256 checksums for every downloadable file.
- A generated SBOM and changelog highlighting consensus-impacting changes.

Always verify signatures and checksums before running binaries.

---

## Downloads & Installation

- **Latest downloads:** [GitHub Releases](https://github.com/Tsoympet/BlockChainDrachma/releases) (tar.gz/AppImage for Linux, `.zip`/`.exe` for Windows, `.dmg` for macOS).
- **Build from source:** See [`docs/building.md`](docs/building.md) for platform-specific setup and Qt deployment tips.
- **Docker quickstart:**
  ```bash
  docker-compose up -d
  docker-compose logs -f drachma-seed-a
  ```
  Use `Dockerfile` for a single-node image or the compose stack for a multi-node testnet with monitoring.

---

## Quick Start / Getting Started

1. **Install prerequisites:** CMake (>=3.18), a C++17 toolchain, OpenSSL, Boost, and system dependencies for your OS. For GPU miners install **CUDA** or **OpenCL** SDKs and matching drivers.
2. **Clone and configure the build:**
   ```bash
   git clone https://github.com/Tsoympet/BlockChainDrachma.git
   cd BlockChainDrachma
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
3. **Compile:**
   ```bash
   cmake --build build --parallel
   ```
4. **(Optional) Run tests:**
   ```bash
   ctest --test-dir build
   ```

> Tip: See [`docs/building.md`](docs/building.md) and [`docs/mining-guide.md`](docs/mining-guide.md) for platform-specific details, GPU tuning, Docker usage, and troubleshooting.

---

## Running the Node, Miner, and Wallet

- **Start a testnet node:**
  ```bash
  ./build/layer1-core/drachmad --datadir ~/.drachma --network testnet --listen --rpcuser=user --rpcpassword=pass
  ```
- **Connect the desktop wallet (Layer 3 app):**
  ```bash
  ./build/layer3-app/drachma-wallet --connect 127.0.0.1:9333
  ```
- **CPU mining to your node:**
  ```bash
  ./build/miners/cpu-miner/drachma-cpuminer --rpc http://user:pass@127.0.0.1:8332 --threads 4
  ```
- **GPU mining (CUDA example):**
  ```bash
  ./build/miners/gpu-miner/drachma-cuda --url 127.0.0.1:9333 --user user --pass pass --intensity 22
  ```
- **Send a transaction (RPC):**
  ```bash
  curl --user user:pass --data-binary '{"method":"sendtoaddress","params":["<DRM-address>", 1.0]}' \
    -H 'content-type: text/plain;' http://127.0.0.1:8332/
  ```

Commands are subject to change as the implementation matures; prefer scripts in `scripts/` for reproducible setups.

## Smart Contracts, NFTs & dApps (optional sidechain)

DRACHMA keeps the base layer minimal. A merge-mined PoW sidechain adds a **modified Ethereum-style EVM** with reduced opcodes, Schnorr signatures, and wDRM gas. The desktop wallet now exposes this optional stack via the **Sidechain** tab:

- **Balances:** view mainnet DRM and sidechain wDRM separately.
- **Bridge:** lock DRM to mint wDRM; burn wDRM to unlock DRM using the peg bridge.
- **Smart contracts:** paste ABI JSON or use ERC-20/721 presets, then call or send transactions to contracts.
- **NFTs:** browse owned tokens, transfer them, mint with metadata URIs (IPFS gateway supported), and manage approvals/operators.
- **dApps:** launch a built-in WebEngine browser pointed at a local or configurable gateway (default `http://localhost:8080`).

Sidechain support can be disabled entirely in **Settings → Sidechain support**. When disabled, the tab hides and the mainnet wallet remains unchanged.

### Live Testnet

The public testnet is online for wallet testing, mining experiments, and RPC integrations.

- **Bootstrap peers:** `testnet/seeds.json` lists live DNS/IP seeds such as `tn1.drachma.org:19335`, `tn2.drachma.org:19335`, and geographically diverse IPs. Nodes automatically attempt these during startup.
- **One-line start (with discovery and RPC):**
  ```bash
  ./build/layer1-core/drachmad --network testnet --datadir ~/.drachma-testnet \
    --addnode tn1.drachma.org:19335 --addnode tn2.drachma.org:19335 \
    --listen --rpcuser=user --rpcpassword=pass --prune=550
  ```
- **Faucet:** request DRM from the testnet faucet using `python3 testnet/faucet.py <address> --amount 5 --rpc http://user:pass@127.0.0.1:18332` (rate limited).
- **Explorer:** a community-run explorer is linked from [`explorer/`](explorer/) for height and mempool visibility.
- **Resetting:** delete `~/.drachma-testnet/blocks` and `chainstate` if you need to resync during protocol updates.

---

## Docker Support

- **Build images:**
  ```bash
  docker build -t drachma/base -f Dockerfile .
  ```
- **Run services with docker-compose:**
  ```bash
  docker-compose up -d
  ```
  The compose file wires Layer 1, RPC services, and a reference miner for quick local testing.
- **Override configs:** mount your own `testnet/` or `scripts/` configuration files via `-v` binds or compose overrides for reproducible environments.

Docker artifacts live alongside the source (`Dockerfile`, `docker-compose.yml`) to keep container recipes auditable.

---

## Usage Examples

- **Start a full node (testnet):**
  ```bash
  ./build/layer1-core/drachmad --datadir ~/.drachma --network testnet --listen --rpcuser=user --rpcpassword=pass
  ```
- **Send a transaction (RPC):**
  ```bash
  curl --user user:pass --data-binary '{"method":"sendtoaddress","params":["<DRM-address>", 1.0]}' \
    -H 'content-type: text/plain;' http://127.0.0.1:8332/
  ```
- **Mine to your node:**
  ```bash
  ./build/miners/gpu-miner/drachma-cuda --url 127.0.0.1:9333 --user user --pass pass --intensity 22
  ```
- **Launch the desktop UI:**
  ```bash
  ./build/layer3-app/drachma-wallet --connect 127.0.0.1:9333
  ```

Commands are subject to change as the implementation matures; prefer scripts in `scripts/` for reproducible setups.

### UI Snapshots

The Layer 3 desktop wallet is testnet-ready. Representative views are available via reproducible demo scripts rather than inline binary assets:

- **Dashboard and sync status:** launch `./build/layer3-app/drachma-wallet --connect 127.0.0.1:9333` and use the built-in “Demo Mode” under **Help → Demo Screens** to generate on-demand previews.
- **Send flow with custom fee selection:** the same demo menu exports a markdown report (`docs/ui_snapshots.md`) describing the send dialog, fee slider, and QR rendering for sharing without embedding binary images in the repository.

---

## Mainnet Launch

Use this checklist before connecting to mainnet or distributing binaries:

- **Build type:** Use release builds with assertions enabled when possible: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDRACHMA_BUILD_TESTS=OFF`.
- **Reproducibility:** Build from a tagged release, pin dependency versions, and verify hashes of toolchains/SDKs.
- **Key hygiene:** Keep the signing key offline; verify maintainer signatures on tags, source archives, and SBOMs.
- **Network settings:** Start nodes with explicit flags (`--network mainnet`, `--listen`, `--rpcuser`, `--rpcpassword`) and review `deployment.md` for hardening.
- **Bootstrap safety:** Prefer initial block download over external snapshots; if using bootstrap files, verify signatures and perform full validation.
- **Miner configuration:** Point miners to authenticated endpoints only, with TLS or trusted LAN where available; review pool settings and difficulty floors.
- **Operational readiness:** Enable logging/rotation, monitor resource usage, and document incident response contacts for your deployment.

## Known Limitations

- **API surface is evolving:** Some RPC methods are still stabilizing; see [`docs/api-reference.md`](docs/api-reference.md) for current coverage and examples.
- **Testnet-first posture:** Monitoring dashboards and alert thresholds are tuned for testnet; operators must adjust for mainnet scale.
- **Hardware wallet support:** Integrations are experimental and should be treated as beta until external audits are complete.
- **OpenCL miner variance:** Performance and determinism can differ across vendor drivers; CUDA paths are prioritized for reproducibility.
- **Docs in motion:** Roadmaps and technical parameters may shift after audit feedback; always consult the latest tagged release notes.

Mainnet procedures and host preparation steps are detailed in [`docs/deployment.md`](docs/deployment.md) and security reviews are outlined in [`docs/audit-guide.md`](docs/audit-guide.md).

---

## Security

- Review the [Security Policy](SECURITY.md) for responsible disclosure and response timelines.
- Consult the [Security Audit Guide](docs/audit-guide.md) for scope, fuzzing targets, and testnet/regtest setups.
- See the [Threat Model](docs/threat-model.md) for adversaries, assets, and mitigations guiding validation and monitoring.

Security-impacting changes and reports are welcomed; consensus/crypto modifications require additional review as noted in [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Core Principles

- **Proof-of-Work:** SHA-256d (double SHA-256), unmodified
- **Launch Model:** No premine, no privileged rewards, no special launch logic
- **Supply Cap:** 42,000,000 DRM
- **Consensus First:** All critical rules reside exclusively in Layer 1
- **No Governance Logic:** No voting systems, no administrative keys
- **No Smart Contracts:** Monetary network only

Network neutrality is achieved through **absence of privilege**, not through special enforcement mechanisms.

---

## Architecture Overview

The system is divided into **three strictly separated layers**:

```mermaid
graph LR
  L1[Layer 1\nConsensus Core] -->|validated blocks & UTXO| L2[Layer 2\nServices]
  L2 -->|RPC, wallets, mempool| L3[Layer 3\nDesktop App]
  L1 -->|mining RPC| Miners[Reference Miners]
```

### Layer 1 — Core Blockchain (Consensus-Critical)
Location: `layer1-core/`

Responsibilities:
- Block and transaction validation
- Proof-of-Work (SHA-256d)
- Difficulty adjustment
- UTXO accounting
- Merkle tree construction
- Schnorr signature verification
- Persistent storage (blocks and chainstate)

Changes in this layer require extreme caution and full review.

---

### Layer 2 — Services (Non-Consensus)
Location: `layer2-services/`

Responsibilities:
- Peer-to-peer networking
- RPC interface
- Mempool management and fee policy
- Wallet backend services
- Transaction indexing
- Cross-chain interoperability (proof-based, non-consensus)

This layer must never alter consensus rules.

---

### Layer 3 — Desktop Application
Location: `layer3-app/`

Responsibilities:
- Graphical desktop application
- Wallet interface
- Miner control
- User-facing configuration
- Asset loading (icons, documentation, legal text)

This layer contains no consensus logic.

---

## Launch Characteristics

The network launches without:
- pre-allocation of supply,
- privileged mining phases,
- protocol-enforced launch conditions,
- or embedded checkpoints.

Mining and block production begin normally from the first block.

Launch conditions are documented in:

- [`docs/fair-launch.md`](docs/fair-launch.md)
- [`docs/security.md`](docs/security.md)
- [`docs/whitepaper.md`](docs/whitepaper.md)

---

## Cryptography

- **Proof-of-Work Hashing:** SHA-256d
- **Merkle Trees:** SHA-256d
- **Transaction Hashing:** Tagged SHA-256
- **Digital Signatures:** Schnorr (secp256k1)

Legacy signature schemes are intentionally excluded.

---

## Mining

Reference mining implementations are provided:
- CPU miner
- GPU miner (CUDA)
- GPU miner (OpenCL)

These miners serve as transparent reference tools and are not optimized black-box software. See [`docs/mining-guide.md`](docs/mining-guide.md) for configuration tips, intensity tuning, and monitoring guidance.

---

## Repository Structure

```
drachma-blockchain/
├─ layer1-core/ # Consensus-critical logic
├─ layer2-services/ # Networking, RPC, wallet backend, cross-chain
├─ layer3-app/ # Desktop application
├─ miners/ # Reference CPU & GPU miners
├─ testnet/ # Test network parameters
├─ docs/ # Technical documentation
├─ tests/ # Test skeletons
├─ common/ # Shared utilities
└─ scripts/ # Build and run scripts
```

---

## Roadmap (Proposed)

- ✅ Establish repository structure and CI for skeleton components
- 🔄 Complete Layer 1 validation logic and state transitions
- 🔄 Harden P2P networking, mempool policy, and wallet services
- 🔄 Finalize reference miners with reproducible build scripts (CPU/GPU)
- 🔄 Launch public testnet with monitoring dashboards, seed nodes, and faucet
- 🔄 Independent external security review of consensus and networking code
- 🔄 Release candidate binaries and deterministic build reproducibility
- 🛠️ Mainnet launch following testnet stability, audits, and reproducible builds

More detail is available in [`docs/roadmap.md`](docs/roadmap.md).

---

## Status

This repository currently contains a **complete structural skeleton** with placeholders.
Implementation is intended to be added incrementally with full review of consensus-critical components.

---

## FAQ

**Why SHA-256d and Schnorr instead of a novel hash or signature scheme?**

- Proven cryptographic primitives with extensive peer review lower systemic risk.
- Hardware and library support are mature, improving performance and auditability.

**Why no smart contracts or on-chain governance?**

- DRACHMA aims to be a minimal monetary network with transparent, predictable rules.
- Avoiding programmability reduces attack surface and consensus complexity.

**What is the total supply and issuance schedule?**

- Hard cap of **42,000,000 DRM**.
- Block subsidy declines on a predictable schedule (see [`docs/technical-spec.md`](docs/technical-spec.md) for parameters) to encourage long-term participation.

**How is the fair launch verifiable?**

- Genesis parameters, launch scripts, and seed configuration live in `testnet/` and `docs/fair-launch.md` so anyone can reproduce the initial state.
- No embedded checkpoints or privileged keys exist; all nodes follow the same rules from block one.

**Is there a premine or privileged allocation?**

- No. DRACHMA follows a **fair launch** model—every coin is mined under the same rules.

**Which platforms are supported?**

- Linux is the primary development environment. macOS and Windows builds are expected but may require additional tooling.

**How do I report security issues?**

- Please follow [`SECURITY.md`](SECURITY.md) for private reporting. Never open public issues for security vulnerabilities.

---

## Community & Support

- **Website:** [https://drachma.org](https://drachma.org) (roadmap, binaries, and trusted mirrors)
- **GitHub Discussions:** [https://github.com/Tsoympet/BlockChainDrachma/discussions](https://github.com/Tsoympet/BlockChainDrachma/discussions)
- **Matrix (bridged to IRC):** [https://matrix.to/#/#drachma:matrix.org](https://matrix.to/#/#drachma:matrix.org)
- **Discord:** [https://discord.gg/drachma](https://discord.gg/drachma) (invite placeholder until public launch)
- **Mailing list:** [mailto:announce@drachma.org](mailto:announce@drachma.org) for release/testnet notices
- **Status page:** [https://status.drachma.org](https://status.drachma.org) for seed/faucet uptime during testnet
- **X/Twitter:** [https://x.com/drachma_org](https://x.com/drachma_org) for short-form status updates

Use Issues/PRs for development work and follow [`CONTRIBUTING.md`](CONTRIBUTING.md) for coding standards and review expectations.

For development coordination, please prefer issues/PRs and follow [`CONTRIBUTING.md`](CONTRIBUTING.md).

---

## License

This project is released under the MIT License.
See `LICENSE` for details.
