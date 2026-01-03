# TLN-Backed NFT State

NFT minting, ownership, and transfers are executed in the TLN domain (`asset_id=0`) for gas only. Metadata hashes, canon references, creators, mint height, and immutable `royalty_bps` live in an isolated state tree. Marketplace flows (`list_nft`, `place_nft_bid`, `settle_nft_sale`) settle strictly in DRM or OBL and automatically route creator royalties; TLN is rejected as a payment currency.
