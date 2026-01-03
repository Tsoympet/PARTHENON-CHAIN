# Layer-2 NFT State (asset-agnostic)

NFT minting, ownership, and transfers are executed in a dedicated Layer-2 NFT domain that does not bind to TLN or any other asset. Metadata hashes, canon references, creators, mint height, and immutable `royalty_bps` live in an isolated state tree. Marketplace flows (`list_nft`, `place_nft_bid`, `settle_nft_sale`) settle strictly in DRM or OBL and automatically route creator royalties; TLN is rejected as a payment currency and never mixed into NFT accounting.
