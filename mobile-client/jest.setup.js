import 'react-native-gesture-handler/jestSetup';

// Mock react-native-reanimated
jest.mock('react-native-reanimated', () => {
  const Reanimated = require('react-native-reanimated/mock');
  Reanimated.default.call = () => {};
  return Reanimated;
});

// Mock react-native-keychain
jest.mock('react-native-keychain', () => ({
  SECURITY_LEVEL_ANY: 'MOCK_SECURITY_LEVEL_ANY',
  SECURITY_LEVEL_SECURE_SOFTWARE: 'MOCK_SECURITY_LEVEL_SECURE_SOFTWARE',
  SECURITY_LEVEL_SECURE_HARDWARE: 'MOCK_SECURITY_LEVEL_SECURE_HARDWARE',
  setGenericPassword: jest.fn(() => Promise.resolve(true)),
  getGenericPassword: jest.fn(() => Promise.resolve({username: 'test', password: 'test'})),
  resetGenericPassword: jest.fn(() => Promise.resolve(true)),
}));

// Mock react-native-mmkv
jest.mock('react-native-mmkv', () => ({
  MMKV: jest.fn(() => ({
    set: jest.fn(),
    getString: jest.fn(),
    delete: jest.fn(),
    clearAll: jest.fn(),
  })),
}));

// Mock react-native-biometrics
jest.mock('react-native-biometrics', () => ({
  simplePrompt: jest.fn(() => Promise.resolve({success: true})),
  isSensorAvailable: jest.fn(() => Promise.resolve({available: true, biometryType: 'FaceID'})),
}));

// Mock @env with default values matching .env.example
// Update these if .env.example changes
jest.mock('@env', () => ({
  DEFAULT_NETWORK: process.env.DEFAULT_NETWORK || 'testnet',
  TESTNET_RPC_URL: process.env.TESTNET_RPC_URL || 'https://tn1.drachma.org:18332',
  MAINNET_RPC_URL: process.env.MAINNET_RPC_URL || 'https://node1.drachma.org:8332',
  APP_NAME: process.env.APP_NAME || 'Drachma Wallet',
  APP_VERSION: process.env.APP_VERSION || '0.1.0',
  MOBILE_MINING_ENABLED: process.env.MOBILE_MINING_ENABLED || 'false',
  POOL_URL: process.env.POOL_URL || '',
  WORKER_NAME: process.env.WORKER_NAME || 'mobile-miner-1',
}));

// Silence the warning: Animated: `useNativeDriver` is not supported
jest.mock('react-native/Libraries/Animated/NativeAnimatedHelper');
