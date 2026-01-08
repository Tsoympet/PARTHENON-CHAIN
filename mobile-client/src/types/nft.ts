/**
 * NFT-related TypeScript types
 */

export interface NFTMetadata {
  id: string;
  tokenId: string;
  name: string;
  description?: string;
  image?: string;
  imageUrl?: string;
  collection?: string;
  contractAddress?: string;
  owner?: string;
  attributes?: NFTAttribute[];
}

export interface NFTAttribute {
  trait_type: string;
  value: string | number;
}

export interface NFTState {
  items: NFTMetadata[];
  isLoading: boolean;
  error: string | null;
}
