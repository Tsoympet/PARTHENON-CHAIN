/**
 * Wallet Service for PARTHENON CHAIN Mobile Wallet
 * 
 * Handles key management, transaction signing, and wallet operations.
 * Uses BIP39 for mnemonic generation and HD wallet derivation.
 */

import {mnemonicToSeedSync, generateMnemonic, validateMnemonic} from '@scure/bip39';
import {wordlist} from '@scure/bip39/wordlists/english';
import {HDKey} from '@scure/bip32';
import {secp256k1} from '@noble/secp256k1';
import {Buffer} from 'buffer';
import {SecureStorage} from '../storage/SecureStorage';

export interface WalletAccount {
  address: string;
  publicKey: string;
  privateKey?: string; // Only stored encrypted
  derivationPath: string;
  index: number;
}

export interface WalletData {
  mnemonic?: string; // Encrypted
  accounts: WalletAccount[];
  currentAccountIndex: number;
}

export class WalletService {
  private static WALLET_KEY = 'drachma_wallet_data';
  private storage: SecureStorage;

  constructor() {
    this.storage = new SecureStorage();
  }

  /**
   * Generate a new wallet with BIP39 mnemonic
   */
  async generateWallet(passphrase = ''): Promise<string> {
    const mnemonic = generateMnemonic(wordlist, 256);
    const seed = mnemonicToSeedSync(mnemonic, passphrase);
    const account = await this.deriveAccount(seed, 0);

    const walletData: WalletData = {
      mnemonic,
      accounts: [account],
      currentAccountIndex: 0,
    };

    await this.storage.setItem(WalletService.WALLET_KEY, JSON.stringify(walletData));
    return mnemonic;
  }

  /**
   * Restore wallet from mnemonic
   */
  async restoreWallet(mnemonic: string, passphrase = ''): Promise<void> {
    if (!validateMnemonic(mnemonic, wordlist)) {
      throw new Error('Invalid mnemonic phrase');
    }

    const seed = mnemonicToSeedSync(mnemonic, passphrase);
    const account = await this.deriveAccount(seed, 0);

    const walletData: WalletData = {
      mnemonic,
      accounts: [account],
      currentAccountIndex: 0,
    };

    await this.storage.setItem(WalletService.WALLET_KEY, JSON.stringify(walletData));
  }

  /**
   * Derive account from seed using BIP44 path
   */
  private async deriveAccount(seed: Uint8Array, index: number): Promise<WalletAccount> {
    const path = `m/44'/9001'/0'/0/${index}`;
    const master = HDKey.fromMasterSeed(seed);
    const child = master.derive(path);

    if (!child.privateKey) {
      throw new Error('Failed to derive private key');
    }

    const publicKey = secp256k1.getPublicKey(child.privateKey, true);
    const address = this.generateAddress(publicKey);

    return {
      address,
      publicKey: Buffer.from(publicKey).toString('hex'),
      privateKey: Buffer.from(child.privateKey).toString('hex'),
      derivationPath: path,
      index,
    };
  }

  /**
   * Generate Drachma address from public key
   */
  private generateAddress(publicKey: Uint8Array): string {
    const hash = secp256k1.utils.sha256(publicKey);
    return 'drm' + Buffer.from(hash).toString('hex').slice(0, 40);
  }

  /**
   * Get current wallet data
   */
  async getWallet(): Promise<WalletData | null> {
    const data = await this.storage.getItem(WalletService.WALLET_KEY);
    return data ? JSON.parse(data) : null;
  }

  /**
   * Get current account
   */
  async getCurrentAccount(): Promise<WalletAccount | null> {
    const wallet = await this.getWallet();
    if (!wallet || wallet.accounts.length === 0) return null;
    return wallet.accounts[wallet.currentAccountIndex];
  }

  /**
   * Create new account
   */
  async createAccount(): Promise<WalletAccount> {
    const wallet = await this.getWallet();
    if (!wallet || !wallet.mnemonic) {
      throw new Error('No wallet found');
    }

    const seed = mnemonicToSeedSync(wallet.mnemonic);
    const newIndex = wallet.accounts.length;
    const account = await this.deriveAccount(seed, newIndex);
    
    wallet.accounts.push(account);
    await this.storage.setItem(WalletService.WALLET_KEY, JSON.stringify(wallet));
    
    return account;
  }

  /**
   * Switch to different account
   */
  async switchAccount(index: number): Promise<void> {
    const wallet = await this.getWallet();
    if (!wallet || index >= wallet.accounts.length) {
      throw new Error('Invalid account index');
    }
    
    wallet.currentAccountIndex = index;
    await this.storage.setItem(WalletService.WALLET_KEY, JSON.stringify(wallet));
  }

  /**
   * Sign transaction (simplified - implement proper signing in production)
   */
  async signTransaction(txData: any): Promise<string> {
    const account = await this.getCurrentAccount();
    if (!account || !account.privateKey) {
      throw new Error('No account available for signing');
    }

    const privateKey = Buffer.from(account.privateKey, 'hex');
    const txHash = secp256k1.utils.sha256(Buffer.from(JSON.stringify(txData)));
    
    // Sign with Schnorr
    const signature = await secp256k1.schnorr.sign(txHash, privateKey);
    
    return Buffer.from(signature).toString('hex');
  }

  /**
   * Delete wallet (use with caution!)
   */
  async deleteWallet(): Promise<void> {
    await this.storage.removeItem(WalletService.WALLET_KEY);
  }

  /**
   * Check if wallet exists
   */
  async hasWallet(): Promise<boolean> {
    return !!(await this.getWallet());
  }
}

export default WalletService;
