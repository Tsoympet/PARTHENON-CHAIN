/**
 * Wallet-related TypeScript types
 */

export interface WalletAccount {
  address: string;
  publicKey: string;
  label?: string;
  balance: number;
  derivationPath?: string;
}

export interface WalletData {
  mnemonic?: string;
  accounts: WalletAccount[];
  currentAccountIndex: number;
  isLocked: boolean;
}

export interface WalletState {
  isInitialized: boolean;
  currentAddress: string | null;
  balances: AssetBalance[];
  transactions: TransactionSummary[];
  isLoading: boolean;
  error: string | null;
}

export interface TransactionSummary {
  id: string;
  txid?: string;
  type: 'send' | 'receive';
  amount: string;
  assetId: string;
  confirmations: number;
  timestamp: number;
  address: string;
}

export interface AssetBalance {
  assetId: string;
  symbol: string;
  name: string;
  balance: number;
  decimals: number;
  usdValue?: number;
}
