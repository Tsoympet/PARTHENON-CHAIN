# NFTs (TLN gas; DRM/OBL settlement)

NFT minting, ownership, and transfers pay TLN gas in the NFT domain but do not mint or burn TLN/DRM/OBL. Marketplace settlement for value discovery uses DRM or OBL only; TLN is rejected as a payment currency.

## Model
- RPC entrypoints: `mint_nft`, `transfer_nft`, `list_nft`, `place_nft_bid`, `settle_nft_sale` (see `sidechain/rpc/wasm_rpc.*`).
- State isolation: ownership, metadata hashes, canon references, mint height, and immutable `royalty_bps` are stored under the NFT domain; other domains cannot access them.
- Royalties: every sale splits `price` into `royalty_amount = price * royalty_bps / 10,000` (to creator) and `seller_amount` (to seller) in the same asset (DRM/OBL).
- Gas: fixed-cost metering for mint/transfer, paid in TLN.

## Using the Desktop GUI
1. Open **Sidechain → NFTs**. Balances show TLN (NFTs), DRM (contracts), and OBL (dApps).
2. Use **Mint** to register a token ID + metadata hash; the wallet funds TLN automatically.
3. Use **Transfer** to move ownership; the UI will reject mixed-asset attempts.

## Tips
- Keep metadata URIs deterministic and hash them client-side before submission.
- Use the TLN domain only; bridging or wrapping assets for NFTs is disallowed by consensus.
