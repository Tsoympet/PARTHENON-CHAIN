# dApps (OBL, asset_id=2)

The sidechain executes **WASM-only dApps** funded exclusively with OBL. EVM/Web3/ABI tooling is not supported; the execution engine rejects mismatched assets.

## Developing
- Target `wasm32-unknown-unknown` and export deterministic entrypoints (no floating point, no host FS).
- Structure a manifest such as `{"module":"obl.dapp.sample","exports":["query","interact"]}` and call via `call_dapp`.
- Pay gas in OBL; other assets are rejected by `ValidateAssetDomain`.

## Example Flow
- Module: lightweight read/write handlers backed by sidechain state.
- Frontend: browser pointed at the dApp gateway (`http://localhost:8080` by default) that signs OBL-backed requests.
- Security: validate user inputs client-side; surface OBL fee estimates.

## Using the Built-in dApp Browser
1. The **Sidechain → dApps** view is always enabled; configure the gateway URL if needed.
2. Select a shortcut or enter a URL; the embedded view keeps the OBL domain and RPC (`call_dapp`) fixed.
3. OBL balances are displayed alongside TLN/DRM in the Sidechain tab.

## Deployment Tips
- Serve static assets over HTTPS when possible; keep RPC endpoints authenticated.
- Ship the WASM manifest with the frontend so calls stay aligned with the enforced exports.
