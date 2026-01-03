# BlockChainDrachma NFTs

Ancient Greek cultural records are modeled as standalone NFTs. They are **not** fungible tokens, do not mint or burn TLN/DRM/OBL, and never participate in supply or consensus calculations.

## Core State (existence)
- Each NFT stores: `nft_id`, `owner_pubkey`, `creator_pubkey`, `metadata_hash`, `canon_reference_hash`, `mint_block_height`, `royalty_bps`.
- `royalty_bps` is immutable, set only at mint, and capped at 1,000 (10%).
- State is isolated under the NFT domain; block headers anchor `nft_state_root`.

## Minting
- RPC: `mint_nft` (TLN for gas only). Fails if the canon reference or metadata hash is missing or if `royalty_bps` is outside `[0, 1000]`.
- Minting records ownership and creator data without touching asset supply.

## Marketplace & Value Discovery
- RPCs: `list_nft`, `place_nft_bid`, `settle_nft_sale`.
- Payments are **only** in DRM or OBL; TLN is rejected as a payment currency.
- Settlement transfers ownership after payment validation.
- Block headers anchor `market_state_root` for listings/bids and `event_root` for deterministic history.

## Royalties
- On every sale: `royalty_amount = price * royalty_bps / 10,000`.
- Creator receives `royalty_amount`; seller receives `price - royalty_amount`. Payments use the sale currency (DRM/OBL).
- Royalty enforcement is protocol-level and cannot be bypassed by UI.

## Events & History
- Emitted events:
  - `NFT_MINTED(nft_id, creator, royalty_bps, height)`
  - `NFT_TRANSFERRED(nft_id, from, to, height)`
  - `NFT_LISTED(nft_id, seller, asset, price, height)`
  - `NFT_BID_PLACED(nft_id, bidder, asset, price, height)`
  - `NFT_SALE_SETTLED(nft_id, seller, buyer, asset, price, royalty_amount, seller_amount, height)`
- These allow explorers to rebuild ownership history, floor prices, volume, and royalty earnings.

## Safety & Isolation
- NFTs are cultural records only; they never alter PoW/PoS, monetary supply, or balance accounting.
- Sidechain validation rejects blocks lacking valid NFT/market/event roots.
