/**
 * Application Constants
 */

/**
 * Network configuration
 */
export const NETWORKS = {
  MAINNET: {
    name: 'Mainnet',
    rpcUrl: 'https://mainnet-rpc.parthenon-chain.io',
    chainId: 1,
  },
  TESTNET: {
    name: 'Testnet',
    rpcUrl: 'https://testnet-rpc.parthenon-chain.io',
    chainId: 2,
  },
  LOCAL: {
    name: 'Local',
    rpcUrl: 'http://localhost:8545',
    chainId: 31337,
  },
};

/**
 * Default network
 */
export const DEFAULT_NETWORK = NETWORKS.TESTNET;

/**
 * Wallet configuration
 */
export const WALLET_CONFIG = {
  DERIVATION_PATH_PREFIX: "m/44'/0'/0'/0/",
  DEFAULT_GAS_LIMIT: 21000,
  DEFAULT_GAS_PRICE: '1000000000', // 1 Gwei
  MIN_CONFIRMATIONS: 6,
};

/**
 * Mining configuration
 */
export const MINING_CONFIG = {
  MIN_DIFFICULTY: 1,
  MAX_DIFFICULTY: 10,
  TARGET_BLOCK_TIME: 60, // seconds
  REWARD_AMOUNT: '50',
};

/**
 * UI constants
 */
export const UI_CONSTANTS = {
  ANIMATION_DURATION: 300,
  TOAST_DURATION: 3000,
  REFRESH_INTERVAL: 30000, // 30 seconds
  MAX_RETRY_ATTEMPTS: 3,
};

/**
 * Storage keys
 */
export const STORAGE_KEYS = {
  WALLET_DATA: 'drachma_wallet_data',
  SETTINGS: 'drachma_settings',
  NETWORK: 'drachma_network',
  THEME: 'drachma_theme',
};

/**
 * Error messages
 */
export const ERROR_MESSAGES = {
  INVALID_ADDRESS: 'Invalid address format',
  INVALID_AMOUNT: 'Invalid amount',
  INSUFFICIENT_BALANCE: 'Insufficient balance',
  NETWORK_ERROR: 'Network error occurred',
  TRANSACTION_FAILED: 'Transaction failed',
  WALLET_LOCKED: 'Wallet is locked',
  INVALID_MNEMONIC: 'Invalid recovery phrase',
};
