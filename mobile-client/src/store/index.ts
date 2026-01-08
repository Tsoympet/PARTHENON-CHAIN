/**
 * Redux Store Configuration
 */

import {configureStore} from '@reduxjs/toolkit';
import walletReducer from './slices/walletSlice';
import networkReducer from './slices/networkSlice';
import miningReducer from './slices/miningSlice';

export const store = configureStore({
  reducer: {
    wallet: walletReducer,
    network: networkReducer,
    mining: miningReducer,
  },
  middleware: getDefaultMiddleware =>
    getDefaultMiddleware({
      serializableCheck: {
        // Ignore these action types
        ignoredActions: ['wallet/setPrivateData'],
      },
    }),
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
