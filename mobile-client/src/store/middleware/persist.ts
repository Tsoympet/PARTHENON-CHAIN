/**
 * Redux Middleware for persisting state
 */

import {Middleware} from '@reduxjs/toolkit';
import {SecureStorage} from '../../services/storage/SecureStorage';

const storage = new SecureStorage();
const PERSIST_KEY = 'redux_persist';

export const persistMiddleware: Middleware = store => next => action => {
  const result = next(action);
  
  // Persist state after each action (debounce in production)
  const state = store.getState();
  const persistableState = {
    network: state.network,
    // Don't persist sensitive wallet data
  };
  
  storage.setItem(PERSIST_KEY, JSON.stringify(persistableState)).catch(err => {
    console.error('Failed to persist state:', err);
  });
  
  return result;
};

export const loadPersistedState = async () => {
  try {
    const data = await storage.getItem(PERSIST_KEY);
    return data ? JSON.parse(data) : undefined;
  } catch (err) {
    console.error('Failed to load persisted state:', err);
    return undefined;
  }
};
