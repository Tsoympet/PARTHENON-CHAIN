# NFTs (Layer-2, TLN-agnostic)

NFT minting, ownership, and transfers run in a dedicated Layer-2 domain with fixed-cost metering and **no** TLN coupling. Marketplace settlement for value discovery uses DRM or OBL only; TLN is rejected as a payment currency and never appears in NFT metadata, logic, or UI.

## Model
- RPC entrypoints: `mint_nft`, `transfer_nft`, `list_nft`, `place_nft_bid`, `settle_nft_sale` (see `sidechain/rpc/wasm_rpc.*`).
- State isolation: ownership, metadata hashes, canon references, mint height, and immutable `royalty_bps` are stored under the NFT domain; other domains cannot access them.
- Royalties: every sale splits `price` into `royalty_amount = price * royalty_bps / 10,000` (to creator) and `seller_amount` (to seller) in the same asset (DRM/OBL).
- Gas: fixed-cost metering for mint/transfer, accounted deterministically without binding to TLN.
- Icons: `/assets/nft-icons/` keyed by `canon_category` with a runtime fallback to `nft-default.svg`.

## Using the Desktop GUI
1. Open **Sidechain → NFTs**. Balances show DRM and OBL for marketplace pricing; NFTs themselves do not display TLN.
2. Use **Mint** to register a token ID + metadata hash; the wallet funds the Layer-2 request without exposing TLN.
3. Use **Transfer** to move ownership; the UI enforces Layer-2 isolation and refuses mixed-asset pricing.

## Tips
- Keep metadata URIs deterministic and hash them client-side before submission.
- NFTs are standalone Layer-2 cultural records; bridging or wrapping assets for NFTs is disallowed by consensus.
