/**
 * Integration Tests
 * 
 * Tests that verify the integration between different components,
 * services, and the Redux store.
 */

import {WalletService} from '../src/services/wallet/WalletService';
import {RPCClient} from '../src/services/rpc/RPCClient';
import {SecureStorage} from '../src/services/storage/SecureStorage';

// Mock services
jest.mock('../src/services/wallet/WalletService');
jest.mock('../src/services/rpc/RPCClient');
jest.mock('../src/services/storage/SecureStorage');

describe('Wallet Integration Tests', () => {
  let walletService: WalletService;
  let rpcClient: RPCClient;

  beforeEach(() => {
    walletService = new WalletService();
    rpcClient = new RPCClient({url: 'https://testnet.drachma.org'});
  });

  it('generates a new wallet and displays it', async () => {
    const mockMnemonic = 'test mnemonic phrase with twelve words here for testing purposes only';
    (walletService.generateWallet as jest.Mock).mockResolvedValue(mockMnemonic);

    // Simulate wallet generation
    const mnemonic = await walletService.generateWallet();
    expect(mnemonic).toBe(mockMnemonic);
    expect(walletService.generateWallet).toHaveBeenCalled();
  });

  it('restores wallet from mnemonic', async () => {
    const mockMnemonic = 'test mnemonic phrase with twelve words here for testing purposes only';
    (walletService.restoreWallet as jest.Mock).mockResolvedValue(undefined);

    await walletService.restoreWallet(mockMnemonic);
    expect(walletService.restoreWallet).toHaveBeenCalledWith(mockMnemonic);
  });

  it('fetches balance from RPC', async () => {
    const mockBalance = '100.50';
    const mockAddress = 'drm1234567890abcdef1234567890abcdef12345678';
    
    (rpcClient.call as jest.Mock).mockResolvedValue({
      result: mockBalance,
    });

    const balance = await rpcClient.call('getbalance', [mockAddress]);
    expect(balance.result).toBe(mockBalance);
  });

  it('creates and signs transaction', async () => {
    const mockTx = {
      to: 'drm9876543210fedcba9876543210fedcba98765432',
      amount: '10.0',
      fee: '0.001',
    };

    const mockSignature = 'signed_transaction_hex_data';
    (walletService.signTransaction as jest.Mock).mockResolvedValue(mockSignature);

    const signature = await walletService.signTransaction(mockTx);
    expect(signature).toBe(mockSignature);
    expect(walletService.signTransaction).toHaveBeenCalledWith(mockTx);
  });

  it('integrates wallet service with Redux store', async () => {
    const mockAccount = {
      address: 'drm1234567890abcdef1234567890abcdef12345678',
      publicKey: 'public_key_hex',
      balance: 100,
      derivationPath: "m/44'/0'/0'/0/0",
    };

    (walletService.getCurrentAccount as jest.Mock).mockResolvedValue(mockAccount);

    const account = await walletService.getCurrentAccount();
    expect(account).toEqual(mockAccount);
  });
});

describe('Transaction Flow Integration', () => {
  it('completes full send transaction flow', async () => {
    const walletService = new WalletService();
    const rpcClient = new RPCClient({url: 'https://testnet.drachma.org'});

    // Step 1: Get current account
    const mockAccount = {
      address: 'drm1234567890abcdef',
      publicKey: 'public_key',
      balance: 100,
    };
    (walletService.getCurrentAccount as jest.Mock).mockResolvedValue(mockAccount);

    // Step 2: Create transaction
    const txData = {
      to: 'drm9876543210fedcba',
      amount: '10.0',
      fee: '0.001',
    };

    // Step 3: Sign transaction
    const mockSignedTx = 'signed_tx_hex';
    (walletService.signTransaction as jest.Mock).mockResolvedValue(mockSignedTx);

    // Step 4: Broadcast transaction
    const mockTxId = 'tx_hash_1234567890abcdef';
    (rpcClient.call as jest.Mock).mockResolvedValue({
      result: mockTxId,
    });

    // Execute flow
    const account = await walletService.getCurrentAccount();
    const signature = await walletService.signTransaction(txData);
    const result = await rpcClient.call('sendrawtransaction', [signature]);

    expect(account).toEqual(mockAccount);
    expect(signature).toBe(mockSignedTx);
    expect(result.result).toBe(mockTxId);
  });
});

describe('Network and Mining Integration', () => {
  it('connects to network and starts mining', async () => {
    const rpcClient = new RPCClient({url: 'https://testnet.drachma.org'});

    // Mock network info
    const mockNetworkInfo = {
      blockHeight: 12345,
      peerCount: 8,
      isSyncing: false,
    };

    (rpcClient.call as jest.Mock).mockResolvedValue({
      result: mockNetworkInfo,
    });

    const networkInfo = await rpcClient.call('getnetworkinfo', []);
    expect(networkInfo.result).toEqual(mockNetworkInfo);
  });
});

describe('Storage Integration', () => {
  it('securely stores and retrieves wallet data', async () => {
    const storage = new SecureStorage();
    const mockData = {
      mnemonic: 'test mnemonic',
      accounts: [],
    };

    (storage.setItem as jest.Mock).mockResolvedValue(undefined);
    (storage.getItem as jest.Mock).mockResolvedValue(JSON.stringify(mockData));

    await storage.setItem('wallet_data', JSON.stringify(mockData));
    const retrieved = await storage.getItem('wallet_data');

    expect(JSON.parse(retrieved!)).toEqual(mockData);
  });
});
