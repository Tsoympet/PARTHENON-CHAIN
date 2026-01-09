/**
 * Application Constants
 */

import {NetworkInfo} from '../types/network';

/**
 * Network Configurations
 */
export const NETWORKS: Record<string, NetworkInfo> = {
  mainnet: {
    type: 'mainnet',
    name: 'Drachma Mainnet',
    rpcUrl: 'https://mainnet.drachma.org',
    rpcPort: 8332,
    explorer: 'https://explorer.drachma.org',
    chainId: 1,
  },
  testnet: {
    type: 'testnet',
    name: 'Drachma Testnet',
    rpcUrl: 'https://tn1.drachma.org',
    rpcPort: 18332,
    explorer: 'https://testnet-explorer.drachma.org',
    chainId: 3,
  },
  regtest: {
    type: 'regtest',
    name: 'Local Regtest',
    rpcUrl: 'http://127.0.0.1',
    rpcPort: 18443,
    chainId: 0,
  },
};

/**
 * Default Network
 */
export const DEFAULT_NETWORK = NETWORKS.testnet;

/**
 * Asset Definitions
 */
export const ASSETS = {
  DRACHMA: {
    id: 'drm',
    symbol: 'DRM',
    name: 'Drachma',
    decimals: 8,
    icon: 'asset-drm',
  },
  TALANTON: {
    id: 'tln',
    symbol: 'TLN',
    name: 'Talanton',
    decimals: 8,
    icon: 'asset-tln',
  },
  OBOLOS: {
    id: 'obl',
    symbol: 'OBL',
    name: 'Obolos',
    decimals: 8,
    icon: 'asset-obl',
  },
} as const;

/**
 * Local NFT Metadata (offline gallery)
 */
export const LOCAL_NFTS = [
  {
    id: 'nft-0001',
    name: 'Parthenon Dawn',
    tokenId: '0001',
    collection: 'Genesis Collection',
    description: 'Golden sunrise over the Parthenon, minted for early supporters.',
  },
  {
    id: 'nft-0042',
    name: 'Drachma Relic',
    tokenId: '0042',
    collection: 'Historical Series',
    description: 'An illustrated relic representing the birth of the Drachma chain.',
  },
  {
    id: 'nft-0100',
    name: 'Obol Constellation',
    tokenId: '0100',
    collection: 'Celestial Archive',
    description: 'A constellation map tied to the Obol staking cycle.',
  },
] as const;

/**
 * Transaction Fee Tiers
 */
export const FEE_TIERS = {
  slow: {
    label: 'Slow',
    blocksTarget: 6,
    multiplier: 1.0,
  },
  medium: {
    label: 'Medium',
    blocksTarget: 3,
    multiplier: 1.5,
  },
  fast: {
    label: 'Fast',
    blocksTarget: 1,
    multiplier: 2.0,
  },
} as const;

/**
 * Error Messages
 */
export const ERROR_MESSAGES = {
  NETWORK_ERROR: 'Unable to connect to network',
  INSUFFICIENT_BALANCE: 'Insufficient balance for transaction',
  INVALID_ADDRESS: 'Invalid wallet address',
  INVALID_AMOUNT: 'Invalid transaction amount',
  TRANSACTION_FAILED: 'Transaction failed to broadcast',
  WALLET_LOCKED: 'Wallet is locked',
  INVALID_MNEMONIC: 'Invalid recovery phrase',
  STORAGE_ERROR: 'Failed to access secure storage',
  MINING_ERROR: 'Mining operation failed',
  RPC_ERROR: 'RPC connection error',
} as const;

/**
 * App Configuration
 */
export const APP_CONFIG = {
  APP_NAME: 'Drachma Mobile Wallet',
  APP_VERSION: '0.2.0',
  MIN_CONFIRMATIONS: 6,
  DEFAULT_TIMEOUT: 30000,
  MAX_RETRY_ATTEMPTS: 3,
  BALANCE_REFRESH_INTERVAL: 30000, // 30 seconds
  TRANSACTION_POLL_INTERVAL: 10000, // 10 seconds
} as const;

/**
 * Wallet Configuration
 */
export const WALLET_CONFIG = {
  DERIVATION_PATH: "m/44'/9001'/0'/0",
  DEFAULT_ACCOUNT_INDEX: 0,
  MNEMONIC_STRENGTH: 256, // 24 words
  ADDRESS_PREFIX: 'drm',
} as const;

/**
 * Mining Configuration Defaults
 */
export const MINING_DEFAULTS = {
  MAX_BATTERY_DRAIN: 5, // percentage per hour
  ENABLE_ON_BATTERY: false,
  ENABLE_ON_CHARGING: true,
  MIN_BATTERY_LEVEL: 30, // percentage
  MAX_TEMPERATURE: 40, // celsius
  HASH_BATCH_SIZE: 100,
  SLEEP_BETWEEN_BATCHES: 100, // milliseconds
  LOW_POWER_MODE: true,
  BACKGROUND_HASH_BATCH_SIZE: 10,
} as const;

/**
 * UI Constants
 */
export const UI_CONSTANTS = {
  TRANSACTION_LIST_PAGE_SIZE: 20,
  DEBOUNCE_DELAY: 300,
  ANIMATION_DURATION: 200,
  TOAST_DURATION: 3000,
} as const;

/**
 * Regex Patterns
 */
export const PATTERNS = {
  ADDRESS: /^drm[0-9a-f]{40,64}$/i,
  PRIVATE_KEY: /^[0-9a-f]{64}$/i,
  TXID: /^[0-9a-f]{64}$/i,
  AMOUNT: /^\d+(\.\d{1,8})?$/,
} as const;
