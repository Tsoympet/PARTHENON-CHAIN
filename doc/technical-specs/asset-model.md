# Asset Model and Roles

| Asset | `asset_id` | Consensus role                    | Execution role                      |
|-------|------------|-----------------------------------|-------------------------------------|
| TLN   | 0          | PoW-only, 21M cap                 | Monetary base; NFTs are Layer-2 and asset-agnostic |
| DRM   | 1          | PoW-only, 41M cap                 | Smart contracts (`deploy_contract`, `call_contract`) |
| OBL   | 2          | PoW-only, 61M cap                 | Settlement token with fast finality, payment channels, institutional features |

Rules:
- Single-asset execution per transaction is enforced in validation; mixed-asset contexts are rejected. The NFT domain is asset-agnostic and anchored by `nft_state_root`.
- `ValidateAssetDomain` binds execution domains to assets for contracts/settlement; DRM cannot mint NFTs, TLN cannot deploy contracts, and OBL is dedicated to settlement operations.
- Sidechain checkpoints are mandatory and anchor `state_root` and `execution_root` back to Layer 1.
- No wrapped assets or optional sidechain toggles exist; WASM is the only supported execution environment.
- **OBL Settlement Features:** Account-based ledger, sub-5 second finality, payment channels, flat fee model, institutional metadata support.
