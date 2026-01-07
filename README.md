# PARTHENON CHAIN
<img width="1024" height="1024" alt="ChatGPT Image 6 Ιαν 2026, 09_00_59 μ μ" src="https://github.com/user-attachments/assets/0fc0e646-cfcf-4610-817a-ba5de3b93c1b" />

[![CI](https://github.com/Tsoympet/PARTHENON-CHAIN/actions/workflows/ci.yml/badge.svg)](https://github.com/Tsoympet/PARTHENON-CHAIN/actions/workflows/ci.yml)
[![Release](https://github.com/Tsoympet/PARTHENON-CHAIN/actions/workflows/release.yml/badge.svg)](https://github.com/Tsoympet/PARTHENON-CHAIN/actions/workflows/release.yml)
[![codecov](https://codecov.io/gh/Tsoympet/PARTHENON-CHAIN/graph/badge.svg?token=pbNZjIpSXO)](https://codecov.io/gh/Tsoympet/PARTHENON-CHAIN)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/Tsoympet/PARTHENON-CHAIN?style=social)](https://github.com/Tsoympet/PARTHENON-CHAIN/stargazers)
[![GitHub Forks](https://img.shields.io/github/forks/Tsoympet/PARTHENON-CHAIN?style=social)](https://github.com/Tsoympet/PARTHENON-CHAIN/network/members)
[![Open Issues](https://img.shields.io/github/issues/Tsoympet/PARTHENON-CHAIN)](https://github.com/Tsoympet/PARTHENON-CHAIN/issues)
[![Security Policy](https://img.shields.io/badge/Security-Policy-orange)](doc/security/security-overview.md)
[![Audit Status](https://img.shields.io/badge/Audit-In%20preparation-blueviolet)](doc/security/audit-guide.md)
[![Discussions](https://img.shields.io/badge/Discussions-join-blue.svg)](#community--support)
[![Roadmap](https://img.shields.io/badge/Roadmap-See%20plans-8A2BE2)](doc/reference/roadmap.md)

**Master Brand**: PARTHENON CHAIN  
**Theme**: Classical Greece · Value · Order · Timelessness  
**Visual DNA**: Marble, Bronze, Silver, Obsidian

PARTHENON CHAIN is a **Proof-of-Work monetary blockchain** designed for long-term stability, auditability, and minimal trust assumptions. Built on Classical principles of order, permanence, and timeless value.

The project focuses on:
- deterministic consensus rules,
- conservative cryptographic design,
- strict separation of system layers,
- and transparent network launch conditions.

This repository contains the **reference implementation** of the PARTHENON CHAIN network.

> **Status: Testnet Ready** — Public testnet nodes, faucets, and explorers are available. Mainnet launch will follow after public testing and audits.

## Releases

Official binaries are published on the [GitHub Releases page](https://github.com/Tsoympet/PARTHENON-CHAIN/releases) for all major platforms. Each release is tagged (vX.Y.Z) and includes:

### Available Platforms

- **Linux x86_64** - Ubuntu 20.04+ compatible, statically linked where possible
- **macOS x86_64** - Intel Macs, macOS 11+
- **macOS arm64** - Apple Silicon Macs (M1/M2/M3)
- **Windows x86_64** - Windows 10+, automated builds available

### Download & Verify

1. **Download binaries** from [Releases](https://github.com/Tsoympet/PARTHENON-CHAIN/releases)
   - Example: `parthenon-core-v0.1.0-linux-x86_64.tar.gz`
   - Also download the `.sha256` checksum file

2. **Verify checksums** (required for security):
   ```bash
   # Linux/macOS
   sha256sum -c parthenon-core-v0.1.0-linux-x86_64.tar.gz.sha256
   
   # Windows (PowerShell)
   (Get-FileHash parthenon-core-v0.1.0-win-x86_64.zip).Hash -eq (Get-Content parthenon-core-v0.1.0-win-x86_64.zip.sha256).Split()[0]
   ```

3. **Extract and run**:
   ```bash
   tar -xzf parthenon-core-v0.1.0-linux-x86_64.tar.gz
   ./drachmad --version
   ./drachmad --help
   ```

**Important:** Always verify checksums before running binaries. See [Verifying Downloads](doc/verifying-downloads.md) for detailed instructions including GPG signature verification.

### What's Included

Each release archive contains:
- **drachmad** - Core daemon (full node)
- **drachma-cli** - Command-line RPC client
- **drachma_cpu_miner** - CPU mining software
- **README.txt** - Quick start guide
- **LICENSE** - MIT License
- **VERSION** - Version information
- **SHA256SUMS** - Binary checksums

### Release Quality

Every release includes:
- **Reproducible builds** - Built with deterministic flags for independent verification
- **SHA-256 checksums** - For every downloadable file
- **GPG signatures** - Detached signatures (when available)
- **Automated testing** - All tests pass before release
- **Changelog** - Complete list of changes, especially consensus-impacting ones

### Building Releases

For reproducible builds from source:
```bash
./scripts/reproducible-build.sh
```

For Gitian deterministic builds:
```bash
# See contrib/gitian-descriptors/README.md
cd gitian-builder
./bin/gbuild ../PARTHENON-CHAIN/contrib/gitian-descriptors/gitian-linux.yml
```

See [Reproducible Builds Guide](doc/reproducible-builds.md) and [Gitian Building](contrib/gitian-descriptors/README.md) for details.

---

## Downloads & Installation

### Pre-built Binaries (Recommended)

Download the latest release for your platform:

**[→ Download Latest Release](https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest)**

Available platforms:
- Linux x86_64 (Ubuntu 20.04+ compatible)
- macOS x86_64 (Intel Macs)
- macOS arm64 (Apple Silicon)
- Windows x86_64 (Windows 10+)

Quick install:
```bash
# Download (example for Linux)
wget https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-linux-x86_64.tar.gz
wget https://github.com/Tsoympet/PARTHENON-CHAIN/releases/latest/download/parthenon-core-v0.1.0-linux-x86_64.tar.gz.sha256

# Verify checksum
sha256sum -c parthenon-core-v0.1.0-linux-x86_64.tar.gz.sha256

# Extract and install
tar -xzf parthenon-core-v0.1.0-linux-x86_64.tar.gz
sudo install -m 755 drachmad drachma-cli drachma_cpu_miner /usr/local/bin/

# Run
drachmad --help
```

See [Installation Guide](doc/install.md) for detailed platform-specific instructions.

### Build from Source

**Option 1: Simple build (recommended)**
```bash
git clone https://github.com/Tsoympet/PARTHENON-CHAIN.git
cd PARTHENON-CHAIN
make
sudo make install
```

**Option 2: Platform-specific installers**
- Linux: `./scripts/install-linux.sh`
- macOS: `./scripts/install-macos.sh`
- Windows: `.\scripts\install-windows.ps1`

**Option 3: Advanced build**

See [`doc/getting-started/building.md`](doc/getting-started/building.md) for:
- Platform-specific dependencies
- CMake build options
- Qt GUI compilation
- GPU miner compilation

**Option 4: Docker**
```bash
docker-compose up -d
docker-compose logs -f drachma-seed-a
```

Use `Dockerfile` for single-node or compose stack for multi-node testnet with monitoring.

---

## Quick Start / Getting Started

**Option 1: Simple Installation (Recommended)**
```bash
# Clone the repository
git clone https://github.com/Tsoympet/PARTHENON-CHAIN.git
cd PARTHENON-CHAIN

# Build and install (Linux/macOS)
make
sudo make install

# Or use the installation script
./scripts/install-linux.sh    # Linux
./scripts/install-macos.sh     # macOS
.\scripts\install-windows.ps1  # Windows (PowerShell)
```

**Option 2: Manual Build**
1. **Install prerequisites:** CMake (>=3.18), a C++17 toolchain, OpenSSL, Boost, and system dependencies for your OS. For GPU miners install **CUDA** or **OpenCL** SDKs and matching drivers.
2. **Clone and configure the build:**
   ```bash
   git clone https://github.com/Tsoympet/PARTHENON-CHAIN.git
   cd PARTHENON-CHAIN
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
3. **Compile:**
   ```bash
   cmake --build build --parallel
   ```
4. **Install (optional):**
   ```bash
   sudo cmake --build build --target install
   ```
5. **Run tests:**
   ```bash
   make test
   # or
   ctest --test-dir build
   ```

> Tip: See [`doc/INSTALL.md`](doc/INSTALL.md) for detailed installation options, [`doc/getting-started/building.md`](doc/getting-started/building.md) for platform-specific details, and [`doc/user-guides/mining-guide.md`](doc/user-guides/mining-guide.md) for GPU tuning and troubleshooting.

---

## Running the Node, Miner, and Wallet

- **Start a testnet node (multi-asset aware):**
  ```bash
  ./build/layer1-core/drachmad --datadir ~/.drachma --network testnet --listen --rpcuser=user --rpcpassword=pass
  ```
- **Query balances per asset (TLN/DRM/OBL) and staking state:**
  ```bash
  ./build/layer1-core/drachma-cli -rpcuser=user -rpcpassword=pass getbalance
  ./build/layer1-core/drachma-cli -rpcuser=user -rpcpassword=pass getbalance \"DRM\"
  ./build/layer1-core/drachma-cli -rpcuser=user -rpcpassword=pass getstakinginfo
  ```
- **Connect the desktop wallet (Layer 3 app):**
  ```bash
  ./build/layer3-app/drachma-wallet --connect 127.0.0.1:9333
  ```
- **CPU mining to your node (TLN-only PoW):**
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

## Smart Contracts, NFTs & dApps (mandatory WASM sidechain)

PARTHENON CHAIN ships a **mandatory WASM execution layer** that is anchored to Layer 1 checkpoints. Domains are enforced by consensus and validation:

| Domain            | Asset use                     | RPC entrypoints                                                        |
|-------------------|-------------------------------|------------------------------------------------------------------------|
| NFTs (Layer 2)    | Asset-agnostic; anchored via `nft_state_root` | `mint_nft`, `transfer_nft`, `list_nft`, `place_nft_bid`, `settle_nft_sale` |
| Smart contracts   | DRM (`asset_id=1`)            | `deploy_contract`, `call_contract`                                     |
| Settlement operations | OBL (`asset_id=2`)          | `obl_sendTransaction`, `obl_sendPathPayment`, `obl_openChannel`       |

- **Execution:** Deterministic WASM only; no EVM/ABI/solidity or wrapped assets. NFT records are standalone cultural entries, settled with fixed gas and never inflate TLN/DRM/OBL supply.
- **Marketplace:** NFT value discovery happens on-chain in DRM or OBL only; royalties are enforced at settlement and paid automatically to creators.
- **Anchors:** Sidechain checkpoints are required; they cannot be disabled in the wallet or node.
- **Wallet UX:** The Sidechain tab surfaces Layer-2 NFTs as first-class records without showing TLN, DRM for contracts, and OBL for settlement operations; it displays the WASM manifest instead of ABI JSON.
- **Settlement:** OBL provides account-based settlement with sub-5 second finality, payment channels, and institutional features. No dApp execution on OBL.
- **Invariant:** NFTs are standalone Layer-2 records and are not connected to TLN in any way.

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
- **Mobile Wallet (iOS/Android):**
  ```bash
  cd mobile-client
  npm install
  # For iOS
  npx react-native run-ios
  # For Android
  npx react-native run-android
  ```
  See [`mobile-client/README.md`](mobile-client/README.md) for detailed setup and features.

Commands are subject to change as the implementation matures; prefer scripts in `scripts/` for reproducible setups.

### UI Snapshots

The Layer 3 desktop wallet is testnet-ready. Representative views are available via reproducible demo scripts rather than inline binary assets:

- **Dashboard and sync status:** launch `./build/layer3-app/drachma-wallet --connect 127.0.0.1:9333` and use the built-in “Demo Mode” under **Help → Demo Screens** to generate on-demand previews.
- **Send flow with custom fee selection:** the same demo menu exports a markdown report (`doc/assets/ui-snapshots.md`) describing the send dialog, fee slider, and QR rendering for sharing without embedding binary images in the repository.

## Internal UI Icon System

- Location: `/assets/ui-icons/` (with light/dark variants) supplies the unified icon set consumed by the Qt wallet. Icons are loaded at runtime; nothing is hardcoded or embedded in binaries.
- Coverage: wallet & funds (wallet/receive/send/balance/address-book/qr), transactions (tx-in/out/pending/confirmed/failed/mempool/history), assets (asset-tln/asset-drm/asset-obl), staking (staking/active/inactive/rewards/lock/unlock/validator), mining (mining/hash/block/difficulty), network (network/peers/sync/synced/warning/error/info/shield), and system (settings/security/key/backup/restore/disk/cpu/memory/log).
- Core vs asset vs UI icons: **core icons** (e.g., splash/app logo) brand the application, **asset icons** represent specific tokens (DRM/OBL/TLN) where appropriate, while **UI icons** are neutral controls and status glyphs reused across menus, tabs, dialogs, and balance/staking/mining/network indicators.
- NFT icons live under `/assets/nft-icons/` and are selected dynamically by `canon_category` with a fallback to `nft-default.svg`; the wallet loads them at runtime and never embeds TLN symbols.

---

## Mainnet Launch

### Infrastructure Deployment

PARTHENON CHAIN now includes production-ready infrastructure tools:

**Seed Nodes:**
```bash
# Deploy a seed node
sudo ./scripts/deploy-seed-node.sh seed-node-1 mainnet

# Start and enable
sudo systemctl start drachma-seed-node-1
sudo systemctl enable drachma-seed-node-1
```
See [Seed Node Deployment Guide](doc/operators/seed-nodes.md) for details.

**Monitoring:**
```bash
# Deploy monitoring stack (Prometheus + Grafana)
docker-compose -f docker-compose.monitoring.yml up -d
```
Access Grafana at http://localhost:3000. See [Monitoring Guide](doc/operators/monitoring.md).

**Extended Testing:**
```bash
# Run 7-day stability validation
python3 scripts/extended-testnet-validation.py \
  --duration 7 \
  --nodes http://node1:18332,http://node2:18332
```

📖 **Quick Reference:** [Mainnet Readiness Guide](doc/MAINNET-READINESS-QUICK-REFERENCE.md)

### Pre-Launch Checklist

Use this checklist before connecting to mainnet or distributing binaries:

- **Build type:** Use reproducible builds: `./scripts/reproducible-build.sh`
- **Reproducibility:** Build from a tagged release, verify with independent build
- **Signatures:** Verify GPG signatures on all binaries and tags
- **Key hygiene:** Keep the signing key offline; verify maintainer signatures on tags, source archives, and SBOMs
- **Network settings:** Start nodes with explicit flags (`--network mainnet`, `--listen`, `--rpcuser`, `--rpcpassword`) and review `deployment.md` for hardening
- **Bootstrap safety:** Prefer initial block download over external snapshots; if using bootstrap files, verify signatures and perform full validation
- **Miner configuration:** Point miners to authenticated endpoints only, with TLS or trusted LAN where available; review pool settings and difficulty floors
- **Operational readiness:** Enable logging/rotation, monitor resource usage, and document incident response contacts for your deployment
- **Infrastructure:** Deploy 3+ seed nodes with monitoring and alerting
- **Testing:** Complete 7+ day extended stability validation

## Known Limitations

- **API surface is evolving:** Some RPC methods are still stabilizing; see [`doc/developer-guides/api-reference.md`](doc/developer-guides/api-reference.md) for current coverage and examples.
- **Testnet-first posture:** Monitoring dashboards and alert thresholds are tuned for testnet; operators must adjust for mainnet scale.
- **Hardware wallet support:** Integrations are experimental and should be treated as beta until external audits are complete.
- **OpenCL miner variance:** Performance and determinism can differ across vendor drivers; CUDA paths are prioritized for reproducibility.
- **Docs in motion:** Roadmaps and technical parameters may shift after audit feedback; always consult the latest tagged release notes.

Mainnet procedures and host preparation steps are detailed in [`doc/operators/deployment.md`](doc/operators/deployment.md) and security reviews are outlined in [`doc/security/audit-guide.md`](doc/security/audit-guide.md).

---

## Security

- Review the [Security Policy](doc/security/security-overview.md) for responsible disclosure and response timelines.
- Consult the [Security Audit Guide](doc/security/audit-guide.md) for scope, fuzzing targets, and testnet/regtest setups.
- See the [Threat Model](doc/security/threat-model.md) for adversaries, assets, and mitigations guiding validation and monitoring.

Security-impacting changes and reports are welcomed; consensus/crypto modifications require additional review as noted in [doc/CONTRIBUTING.md](doc/CONTRIBUTING.md).

---

## Core Principles

- **Proof-of-Work:** SHA-256d (double SHA-256), unmodified
- **Launch Model:** No premine, no privileged rewards, no special launch logic
- **Supply Caps:** 21M TLN, 41M DRM, 61M OBL (all PoW-mined)
- **Consensus First:** All critical rules reside exclusively in Layer 1
- **No Governance Logic:** No voting systems, no administrative keys
- **Execution model:** Mandatory WASM sidechain with enforced domain law (Layer-2 NFTs are asset-agnostic; DRM→contracts; OBL→settlement operations)
- **Settlement Layer:** OBL provides institutional-grade payment settlement with deterministic finality, account-based ledger, and predictable fees

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

### Mobile Client
Location: `mobile-client/`

Responsibilities:
- Cross-platform mobile wallet (iOS and Android)
- Secure key management with biometric authentication
- Multi-asset support (TLN, DRM, OBL)
- NFT gallery and marketplace access
- QR code scanning for easy transactions
- Offline mode for viewing balances

Built with React Native for optimal performance on mobile devices.
See [`mobile-client/README.md`](mobile-client/README.md) for details.

---

## Launch Characteristics

The network launches without:
- pre-allocation of supply,
- privileged mining phases,
- protocol-enforced launch conditions,
- or embedded checkpoints.

Mining and block production begin normally from the first block.

Launch conditions are documented in:

- [`doc/reference/fair-launch.md`](doc/reference/fair-launch.md)
- [`doc/security/security-notes.md`](doc/security/security-notes.md)
- [`doc/reference/whitepaper.md`](doc/reference/whitepaper.md)

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

These miners serve as transparent reference tools and are not optimized black-box software. See [`doc/user-guides/mining-guide.md`](doc/user-guides/mining-guide.md) for configuration tips, intensity tuning, and monitoring guidance.

---

## Repository Structure

```
drachma-blockchain/
├─ layer1-core/       # Consensus-critical logic
├─ layer2-services/   # Networking, RPC, wallet backend, cross-chain
├─ layer3-app/        # Desktop application (Qt-based)
├─ mobile-client/     # Mobile wallet (React Native, iOS/Android)
├─ miners/            # Reference CPU & GPU miners
├─ testnet/           # Test network parameters
├─ doc/               # Technical documentation
├─ tests/             # Unit, integration, and fuzz tests
├─ common/            # Shared utilities
└─ scripts/           # Build and run scripts
```

---

## Roadmap (Proposed)

- ✅ Maintain repository structure and CI across components
- ✅ Complete Layer 1 validation logic and state transitions
- ✅ Harden P2P networking, mempool policy, and wallet services
- ✅ Finalize reference miners with reproducible build scripts (CPU/GPU)
- ✅ Launch public testnet with monitoring dashboards, seed nodes, and faucet
- 🔄 Independent external security review of consensus and networking code
- 🔄 Release candidate binaries and deterministic build reproducibility
- 🛠️ Mainnet launch following testnet stability, audits, and reproducible builds

More detail is available in [`doc/reference/roadmap.md`](doc/reference/roadmap.md).

**📋 Project Completion Tasks:**  
For a comprehensive list of all tasks needed to complete the project and prepare for mainnet launch, see:
- **[Project Status](PROJECT-STATUS.md)** - Complete overview of current state, progress, and timeline ⭐ NEW
- **[Quick Task Summary](doc/QUICK-TASK-SUMMARY.md)** - Quick reference guide
- **[Project Completion Tasks](doc/PROJECT-COMPLETION-TASKS.md)** - Detailed task list with priorities, timelines, and dependencies
- **[Launch Action Items](doc/LAUNCH-ACTION-ITEMS.md)** - Prioritized action plan for mainnet launch
- **[Mainnet Readiness Assessment](doc/MAINNET-READINESS.md)** - Technical readiness evaluation

**🚀 Performance & Branding:**  
For optimization tips and brand guidelines:
- **[Performance Guide](doc/PERFORMANCE-GUIDE.md)** - Comprehensive performance optimization guide with benchmarks
- **[Branding Guide](doc/BRANDING-GUIDE.md)** - Complete branding guidelines, visual identity, and messaging

---

## Status

This repository now ships a working reference implementation with Layer 1/2/3 binaries, miners, and ctests wired into CI. 

**Current Status:** ✅ Testnet Ready | ⚠️ Approaching Mainnet Readiness (8-12 weeks estimated)

Remaining hardening items and edge-case gaps are tracked in:
- [`doc/security/AUDIT.md`](doc/security/AUDIT.md) - Known security issues
- [`doc/PROJECT-COMPLETION-TASKS.md`](doc/PROJECT-COMPLETION-TASKS.md) - All pending tasks

---

## FAQ

**Is the blockchain and core client ready?**

- Yes. The Layer 1 consensus node (`drachmad`) is built from `layer1-core/` and is included in the testnet-ready binaries and Docker compose stack in this repo.
- You can compile it yourself using the Quick Start steps above (`cmake --build build --parallel`) and run it with `./build/layer1-core/drachmad --network testnet ...`.
- Public testnet seeds, faucet, and explorer endpoints are already wired into the default configs under `testnet/` and the compose stack, so you can sync a node or connect the wallet immediately.

**Why SHA-256d and Schnorr instead of a novel hash or signature scheme?**

- Proven cryptographic primitives with extensive peer review lower systemic risk.
- Hardware and library support are mature, improving performance and auditability.

**Why no smart contracts or on-chain governance?**

- PARTHENON CHAIN aims to be a minimal monetary network with transparent, predictable rules.
- Smart contracts are supported via DRM asset for those who need programmability.
- OBL focuses on settlement operations, not general computation.
- Avoiding unnecessary programmability reduces attack surface and consensus complexity.

**What is the total supply and issuance schedule?**

- **TLN (Talanton):** 21,000,000 cap, PoW-only
- **DRM (Drachma):** 41,000,000 cap, PoW-only
- **OBL (Obolos):** 61,000,000 cap, PoW-only
- Block subsidy declines on a predictable schedule (see [`doc/technical-specs/technical-spec.md`](doc/technical-specs/technical-spec.md) for parameters) to encourage long-term participation.
- All assets are mined via Proof-of-Work; no Proof-of-Stake, no premine.

**How is the fair launch verifiable?**

- Genesis parameters, launch scripts, and seed configuration live in `testnet/` and `doc/reference/fair-launch.md` so anyone can reproduce the initial state.
- No embedded checkpoints or privileged keys exist; all nodes follow the same rules from block one.

**Is there a premine or privileged allocation?**

- No. PARTHENON CHAIN follows a **fair launch** model—every coin is mined under the same rules.

**Which platforms are supported?**

- Linux is the primary development environment. macOS and Windows builds are expected but may require additional tooling.

**How do I report security issues?**

- Please follow [`SECURITY.md`](doc/security/security-overview.md) for private reporting. Never open public issues for security vulnerabilities.

---

## Community & Support

- **Website:** [https://drachma.org](https://drachma.org) (roadmap, binaries, and trusted mirrors)
- **GitHub Discussions:** [https://github.com/Tsoympet/PARTHENON-CHAIN/discussions](https://github.com/Tsoympet/PARTHENON-CHAIN/discussions)
- **Matrix (bridged to IRC):** [https://matrix.to/#/#drachma:matrix.org](https://matrix.to/#/#drachma:matrix.org)
- **Discord:** [https://discord.gg/drachma](https://discord.gg/drachma) (invite placeholder until public launch)
- **Mailing list:** [mailto:announce@drachma.org](mailto:announce@drachma.org) for release/testnet notices
- **Status page:** [https://status.drachma.org](https://status.drachma.org) for seed/faucet uptime during testnet
- **X/Twitter:** [https://x.com/drachma_org](https://x.com/drachma_org) for short-form status updates

Use Issues/PRs for development work and follow [`doc/CONTRIBUTING.md`](doc/CONTRIBUTING.md) for coding standards and review expectations.

For development coordination, please prefer issues/PRs and follow [`doc/CONTRIBUTING.md`](doc/CONTRIBUTING.md).

---

## License

This project is released under the MIT License.
See `LICENSE` for details.
