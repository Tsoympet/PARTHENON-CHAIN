/**
 * Transaction-related TypeScript types
 */

export type TransactionType = 'send' | 'receive' | 'contract';
export type TransactionStatus = 'pending' | 'confirmed' | 'failed';

export interface Transaction {
  id: string;
  hash?: string;
  type: TransactionType;
  from: string;
  to: string;
  amount: string;
  fee?: string;
  timestamp: number;
  status: TransactionStatus;
  confirmations?: number;
  blockNumber?: number;
  memo?: string;
}

export interface TransactionRequest {
  to: string;
  amount: string;
  memo?: string;
  gasLimit?: string;
  gasPrice?: string;
}
