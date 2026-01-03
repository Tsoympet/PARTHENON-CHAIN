# BlockChainDrachma Sidechain (WASM) Specification

## Goals
- Deterministic WASM execution with fixed-cost metering.
- Mandatory domain separation:
  - NFTs live in a Layer-2 domain anchored by `nft_state_root` and do **not** couple to TLN or any other asset.
  - DRM (`asset_id=1`) → smart contracts only.
  - OBL (`asset_id=2`) → dApps only.
- Sidechain state is independent from the main-chain UTXO model; checkpoints anchor execution roots back to the main chain.
- NFT, marketplace, and event roots (`nft_state_root`, `market_state_root`, `event_root`) are anchored per-block to keep cultural records and price history immutable.

## Runtime
- Interpreter-only (no JIT) WASM subset with a deterministic instruction schedule and fixed gas costs (see `sidechain/wasm/gas/model.*`).
- Memory limits enforced through stack bounds and explicit memory charging.
- No host OS calls; imports are explicit and limited to storage/crypto/logging stubs.
- Execution domain and paying asset are bound in `ExecutionRequest` and validated by `ValidateAssetDomain`.
- State is stored per-domain/module (`sidechain/state/state_store.*`) to prevent cross-domain privilege escalation.

### Execution Domains
| Domain            | Asset        | Allowed actions                                                                 | Forbidden actions                          |
|-------------------|--------------|---------------------------------------------------------------------------------|--------------------------------------------|
| Smart Contracts   | DRM (`1`)    | Deploy/call WASM contracts, pay gas                                             | Mint/transfer NFTs, execute dApps          |
| NFTs              | None (L2)    | Mint, transfer, lookup ownership/metadata; list/bid/settle marketplace trades with enforced royalties | Deploy/call contracts, run dApps           |
| dApps             | OBL (`2`)    | Lightweight WASM calls for interactions/reads                                   | Deploy DRM contracts, mint/transfer NFTs   |

NFT sale settlement is denominated strictly in DRM or OBL; TLN is rejected as a payment currency. NFT state stays isolated from TLN/DRM/OBL supply and never touches balances.

## Validation Rules
- Reject any execution where `asset_id` does not match the requested domain for DRM/OBL. NFT execution is asset-agnostic by design.
- Reject mixed-asset execution contexts.
- Sidechain blocks must carry:
  - `state_root`
  - `execution_root`
  - `nft_state_root`
  - `market_state_root`
  - `event_root`
  - `main_chain_checkpoint`
  - `height`
- `ValidateCheckpoint` enforces checkpoint matching and presence of execution anchors. Blocks lacking a valid main-chain checkpoint are invalid.

## RPC Endpoints
Implemented in `sidechain/rpc/wasm_rpc.*`:
- `deploy_contract` (DRM): initializes a WASM contract with deterministic gas accounting.
- `call_contract` (DRM): executes contract code; gas paid in DRM.
- `mint_nft`: mints standalone NFT record with creator, canonical reference hash, immutable `royalty_bps` under the NFT domain (asset-agnostic).
- `transfer_nft`: updates ownership; enforces current owner check without touching TLN.
- `list_nft` / `place_nft_bid` / `settle_nft_sale`: marketplace flows that settle in DRM or OBL, automatically splitting royalties to creators.
- `call_dapp` (OBL): runs dApp WASM with OBL gas.

## Directory Layout (mandatory)
```
sidechain/
  wasm/
    runtime/      # deterministic interpreter & types
    gas/          # fixed gas schedule and metering
    validator/    # asset law + checkpoint validation
  contracts/      # DRM contract modules
  nfts/           # Layer-2 NFT modules (asset-agnostic)
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
