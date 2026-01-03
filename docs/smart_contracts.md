# WASM Smart Contracts (DRM, asset_id=1)

The sidechain is **mandatory** and executes deterministic WASM modules only. EVM/ABI/solidity, wrapped assets, and optional toggles are not supported. DRM (`asset_id=1`) is the sole funding asset for contracts.

## Model
- Deterministic interpreter (no JIT), fixed gas schedule (`sidechain/wasm/gas`), and explicit host imports.
- Asset/function law: DRM → smart contracts; OBL → dApps; NFTs run in an asset-agnostic Layer-2 domain. Mixed-asset execution is rejected by `ValidateAssetDomain` for contract/dApp domains.
- Checkpoints are required; every block carries `state_root`, `execution_root`, and a main-chain checkpoint.
- RPC entrypoints: `deploy_contract` and `call_contract` via `sidechain/rpc/wasm_rpc.*`.

## Using the Desktop GUI
1. The **Sidechain** tab is always enabled; RPC defaults to `http://localhost:9334/wasm`.
2. The Smart Contracts pane expects a WASM manifest JSON: `{"module":"<id>","exports":["init","handle_call"]}`.
3. The wallet auto-selects DRM for gas and submits `deploy_contract`/`call_contract` requests with the provided manifest/payload.
4. Checkpoint status and peer count are displayed in the status bar; they cannot be disabled.

## Deploying and Calling
- Build your contract to WASM (no floating point). Keep exports minimal and deterministic.
- Deploy with `deploy_contract` supplying the module ID, code, and gas limit.
- Invoke exported functions with `call_contract`, passing DRM for gas. Calls outside the DRM domain are rejected.

## Tooling
- Prefer Rust/C/C++ toolchains targeting `wasm32-unknown-unknown` with deterministic builds.
- Use the provided `sidechain_wasm_execution_test` as a reference for gas metering and asset-law validation.
