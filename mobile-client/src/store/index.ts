/**
 * Redux Store Configuration
 */

import {combineReducers, configureStore} from '@reduxjs/toolkit';
import {persistReducer, persistStore} from 'redux-persist';
import walletReducer from './slices/walletSlice';
import networkReducer from './slices/networkSlice';
import miningReducer from './slices/miningSlice';
import {persistConfig} from './middleware/persist';

const rootReducer = combineReducers({
  wallet: walletReducer,
  network: networkReducer,
  mining: miningReducer,
});

const persistedReducer = persistReducer(persistConfig, rootReducer);

export const store = configureStore({
  reducer: persistedReducer,
  middleware: getDefaultMiddleware =>
    getDefaultMiddleware({
      serializableCheck: {
        // Ignore these action types
        ignoredActions: ['persist/PERSIST', 'persist/REHYDRATE'],
      },
    }),
});

export const persistor = persistStore(store);

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
