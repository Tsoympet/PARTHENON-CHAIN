# BlockChainDrachma Sidechain (WASM) Specification

## Goals
- Deterministic WASM execution with fixed-cost metering.
- Mandatory asset-to-function separation:
  - TLN (`asset_id=0`) → NFTs only.
  - DRM (`asset_id=1`) → smart contracts only.
  - OBL (`asset_id=2`) → dApps only.
- Sidechain state is independent from the main-chain UTXO model; checkpoints anchor execution roots back to the main chain.

## Runtime
- Interpreter-only (no JIT) WASM subset with a deterministic instruction schedule and fixed gas costs (see `sidechain/wasm/gas/model.*`).
- Memory limits enforced through stack bounds and explicit memory charging.
- No host OS calls; imports are explicit and limited to storage/crypto/logging stubs.
- Execution domain and paying asset are bound in `ExecutionRequest` and validated by `ValidateAssetDomain`.
- State is stored per-domain/module (`sidechain/state/state_store.*`) to prevent cross-domain privilege escalation.

### Execution Domains
| Domain            | Asset        | Allowed actions                                    | Forbidden actions                          |
|-------------------|--------------|----------------------------------------------------|--------------------------------------------|
| Smart Contracts   | DRM (`1`)    | Deploy/call WASM contracts, pay gas                | Mint/transfer NFTs, execute dApps          |
| NFTs              | TLN (`0`)    | Mint, transfer, lookup ownership/metadata          | Deploy/call contracts, run dApps           |
| dApps             | OBL (`2`)    | Lightweight WASM calls for interactions/reads      | Deploy DRM contracts, mint/transfer NFTs   |

## Validation Rules
- Reject any execution where `asset_id` does not match the requested domain.
- Reject mixed-asset execution contexts.
- Sidechain blocks must carry:
  - `state_root`
  - `execution_root`
  - `main_chain_checkpoint`
  - `height`
- `ValidateCheckpoint` enforces checkpoint matching and presence of execution anchors. Blocks lacking a valid main-chain checkpoint are invalid.

## RPC Endpoints
Implemented in `sidechain/rpc/wasm_rpc.*`:
- `deploy_contract` (DRM): initializes a WASM contract with deterministic gas accounting.
- `call_contract` (DRM): executes contract code; gas paid in DRM.
- `mint_nft` (TLN): mints token, stores owner + metadata hash; only TLN accepted.
- `transfer_nft` (TLN): updates ownership; enforces current owner check.
- `call_dapp` (OBL): runs dApp WASM with OBL gas.

## Directory Layout (mandatory)
```
sidechain/
  wasm/
    runtime/      # deterministic interpreter & types
    gas/          # fixed gas schedule and metering
    validator/    # asset law + checkpoint validation
  contracts/      # DRM contract modules
  nfts/           # TLN-backed NFT modules
  dapps/          # OBL-backed dApps
  state/          # isolated state store
  rpc/            # RPC façades for sidechain actions
  sidechain_spec.md
```

## Determinism & Safety
- No floating point; integer-only execution.
- Bounded stack (`kMaxStack`) and metered memory writes.
- Explicit domain tagging prevents cross-domain access to storage or gas.
- Checkpoints allow main-chain nodes to verify sidechain execution roots deterministically.
