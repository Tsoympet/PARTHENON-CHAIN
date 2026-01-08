/**
 * Network-related TypeScript types
 */

export interface NetworkInfo {
  name: string;
  rpcUrl: string;
  chainId: number;
  blockHeight?: number;
  gasPrice?: string;
}

export interface NetworkState {
  current: NetworkInfo;
  connected: boolean;
  blockHeight: number;
  gasPrice: string;
  isLoading: boolean;
  error: string | null;
}
