# Asset Model and Roles

| Asset | `asset_id` | Consensus role                    | Execution role                      |
|-------|------------|-----------------------------------|-------------------------------------|
| TLN   | 0          | PoW-only, 21M cap                 | Monetary base; NFTs are Layer-2 and asset-agnostic |
| DRM   | 1          | PoW + PoS hybrid, 42M cap target  | Smart contracts (`deploy_contract`, `call_contract`) |
| OBL   | 2          | PoS-only, fixed 61M target supply | dApps/interaction fees (`call_dapp`)  |

Rules:
- Single-asset execution per transaction is enforced in validation; mixed-asset contexts are rejected. The NFT domain is asset-agnostic and anchored by `nft_state_root`.
- `ValidateAssetDomain` binds execution domains to assets for contracts/dApps; DRM cannot mint NFTs, TLN cannot deploy contracts, and OBL cannot host DRM contracts.
- Sidechain checkpoints are mandatory and anchor `state_root` and `execution_root` back to Layer 1.
- No wrapped assets or optional sidechain toggles exist; WASM is the only supported execution environment.
