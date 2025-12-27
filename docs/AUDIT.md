# Repository Audit (Build, Logic, and Missing Components)

This document catalogs the most visible gaps in build tooling, consensus/service logic, and supporting assets. The README explicitly notes the repository is a structural skeleton, and the findings below detail what remains unimplemented.

## Build and Installation Tooling
- **No build orchestration**: The root `CMakeLists.txt` only declares the project and has no targets, options, dependencies, tests, or install/export rules. A complete build graph (libraries, executables, tests) is missing. 【F:CMakeLists.txt†L1-L2】
- **Absent installers/scripts**: There are no Makefiles, setup scripts, packaging specs, or CI workflows to drive builds, testing, or deployment. The `scripts/` directory referenced in the README currently lacks runnable orchestration entries.

## Layer 1 (Consensus-Critical) Gaps
- **Block/transaction validation is skeletal**: `ValidateBlock` only checks proof-of-work, merkle root, and basic output shape; it omits coinbase rules, input spending checks against the UTXO set, fee/subsidy validation, or signature/script verification per input. 【F:layer1-core/validation/validation.cpp†L8-L38】
- **Script interpreter lacks UTXO context**: `VerifyScript` assumes the spent output exists inside the same transaction and treats the input’s `scriptSig` as a Schnorr signature over the transaction serialization, with no support for external UTXO data, sighash flags, or malleability protections. 【F:layer1-core/script/interpreter.cpp†L9-L42】

## Layer 2 (Services) Gaps
- **P2P networking is an in-memory stub**: `P2PNode` manages peers, message queues, and handlers entirely in-memory with no socket management, protocol framing, encryption, or peer discovery, so no actual network interoperability exists yet. 【F:layer2-services/net/p2p.cpp†L13-L110】
- **Other services unimplemented**: RPC, mempool, policy, index, and cross-chain modules have no exposed build targets or wiring to core logic, mirroring the README’s “skeleton” status. 【F:README.md†L115-L134】

## Layer 3 (Desktop Application) Gaps
- **UI uses synthetic data**: `NodeController` and `MiningController` generate random peers, synthetic hashrate, and steadily increment block height instead of binding to layer 1/2 nodes or miners, indicating the GUI is a placeholder. 【F:layer3-app/qt/main.cpp†L107-L199】
- **Assets/configuration absent**: The `assets/README.md` notes the folder but no icons, translations, or user-facing configuration bundles are present to ship an application.

## Testing and QA
- **Test stubs without runners**: Although `tests/` contains initial source files, there is no CMake/CTest integration or automation to compile/run them. Coupled with missing build targets, no unit, integration, or regression coverage can execute at present.

## Recommended Next Steps
1. Define full CMake targets for core libraries, services, GUI, miners, and tests; add install/export and toolchain options.
2. Add reproducible build/run scripts (or Makefile) and CI workflows to compile and execute tests across platforms.
3. Flesh out consensus validation (coinbase rules, UTXO lookup, signature hashing modes, fee checks) and connect the script interpreter to real UTXO/state data.
4. Implement real networking stacks (peer handshake, message framing, inventory/relay, banning) and RPC/mempool plumbing.
5. Connect the GUI to service APIs and supply packaged assets/configuration for release builds.

## Implemented Remediations
- Added initial CMake targets for layer 1/2 libraries, the CPU miner, optional Qt GUI, and test executables with install rules to generate usable build artifacts. 【F:CMakeLists.txt†L1-L75】【F:CMakeLists.txt†L94-L101】
- Wired a GitHub Actions CI workflow plus deterministic `scripts/test.sh` to configure, build, and run ctests without the GUI toolchain. 【F:.github/workflows/ci.yml†L1-L23】【F:scripts/test.sh†L1-L13】
- Strengthened block validation with coinbase size rules, subsidy caps, UTXO-backed script checks, and a UTXO-aware interpreter entry point to enforce spends against provided state. 【F:layer1-core/validation/validation.cpp†L17-L86】【F:layer1-core/script/interpreter.cpp†L12-L33】
