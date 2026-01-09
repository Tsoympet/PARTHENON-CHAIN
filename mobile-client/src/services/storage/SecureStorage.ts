/**
 * Secure Storage Service
 *
 * Uses Expo SecureStore to keep sensitive wallet data encrypted
 * in the platform keychain/keystore.
 */

import * as SecureStore from 'expo-secure-store';

export class SecureStorage {
  /**
   * Store item securely
   */
  async setItem(key: string, value: string): Promise<void> {
    await SecureStore.setItemAsync(key, value, {
      keychainAccessible: SecureStore.AFTER_FIRST_UNLOCK,
    });
  }

  /**
   * Retrieve item
   */
  async getItem(key: string): Promise<string | null> {
    return await SecureStore.getItemAsync(key);
  }

  /**
   * Remove item
   */
  async removeItem(key: string): Promise<void> {
    await SecureStore.deleteItemAsync(key);
  }

  /**
   * Clear a list of keys
   */
  async clear(keys: string[]): Promise<void> {
    await Promise.all(keys.map(key => SecureStore.deleteItemAsync(key)));
  }
}

export default SecureStorage;
