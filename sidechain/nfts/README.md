# TLN-Backed NFTs

NFT minting, ownership, and transfers are executed in the TLN domain (`asset_id=0`). Metadata hashes and owners are stored via the isolated state store. Operations are exposed through `mint_nft` and `transfer_nft` RPCs and cannot consume DRM or OBL.
