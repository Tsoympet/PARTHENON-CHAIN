/**
 * Wallet-related TypeScript types
 */

export interface WalletAccount {
  address: string;
  publicKey: string;
  privateKey?: string;
  derivationPath: string;
  index: number;
  balance?: string;
  name?: string;
}

export interface WalletData {
  mnemonic?: string;
  accounts: WalletAccount[];
  currentAccountIndex: number;
  isLocked: boolean;
}

export interface WalletState {
  initialized: boolean;
  locked: boolean;
  address: string | null;
  balance: string | null;
  accounts: WalletAccount[];
  currentAccountIndex: number;
  isLoading: boolean;
  error: string | null;
}
