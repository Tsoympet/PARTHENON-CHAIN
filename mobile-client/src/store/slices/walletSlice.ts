/**
 * Wallet State Management
 */

import {createSlice, PayloadAction} from '@reduxjs/toolkit';
import {AssetBalance} from '../../types/wallet';
import {Transaction} from '../../types/transaction';

interface WalletState {
  isInitialized: boolean;
  currentAddress: string | null;
  balances: AssetBalance[];
  transactions: Transaction[];
  isLoading: boolean;
  error: string | null;
}

const initialState: WalletState = {
  isInitialized: false,
  currentAddress: null,
  balances: [
    {assetId: 'drm', symbol: 'DRM', name: 'Drachma', balance: 0, decimals: 8},
    {assetId: 'obl', symbol: 'OBL', name: 'Obol', balance: 0, decimals: 8},
    {assetId: 'tln', symbol: 'TLN', name: 'Talanton', balance: 0, decimals: 8},
  ],
  transactions: [],
  isLoading: false,
  error: null,
};

const walletSlice = createSlice({
  name: 'wallet',
  initialState,
  reducers: {
    setInitialized: (state, action: PayloadAction<boolean>) => {
      state.isInitialized = action.payload;
    },
    setCurrentAddress: (state, action: PayloadAction<string>) => {
      state.currentAddress = action.payload;
    },
    setBalances: (state, action: PayloadAction<AssetBalance[]>) => {
      state.balances = action.payload;
    },
    updateBalance: (state, action: PayloadAction<AssetBalance>) => {
      const index = state.balances.findIndex(
        b => b.assetId === action.payload.assetId
      );
      if (index >= 0) {
        state.balances[index] = action.payload;
      } else {
        state.balances.push(action.payload);
      }
    },
    setTransactions: (state, action: PayloadAction<Transaction[]>) => {
      state.transactions = action.payload;
    },
    addTransaction: (state, action: PayloadAction<Transaction>) => {
      state.transactions.unshift(action.payload);
    },
    setLoading: (state, action: PayloadAction<boolean>) => {
      state.isLoading = action.payload;
    },
    setError: (state, action: PayloadAction<string | null>) => {
      state.error = action.payload;
    },
    resetWallet: () => initialState,
  },
});

export const {
  setInitialized,
  setCurrentAddress,
  setBalances,
  updateBalance,
  setTransactions,
  addTransaction,
  setLoading,
  setError,
  resetWallet,
} = walletSlice.actions;

export default walletSlice.reducer;
