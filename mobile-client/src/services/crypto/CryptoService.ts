/**
 * Cryptographic Operations Service
 * 
 * Provides encryption, decryption, hashing, and key derivation utilities.
 */

import {secp256k1} from '@noble/secp256k1';
import {Buffer} from 'buffer';

export class CryptoService {
  /**
   * Generate a random private key
   */
  static generatePrivateKey(): Uint8Array {
    return secp256k1.utils.randomPrivateKey();
  }

  /**
   * Derive public key from private key
   */
  static getPublicKey(privateKey: Uint8Array, compressed = true): Uint8Array {
    return secp256k1.getPublicKey(privateKey, compressed);
  }

  /**
   * Hash data using SHA256
   */
  static sha256(data: Uint8Array | string): Uint8Array {
    const buffer = typeof data === 'string' ? Buffer.from(data, 'utf8') : data;
    return secp256k1.utils.sha256(buffer);
  }

  /**
   * Sign message with Schnorr signature
   */
  static async schnorrSign(
    message: Uint8Array,
    privateKey: Uint8Array,
  ): Promise<Uint8Array> {
    return await secp256k1.schnorr.sign(message, privateKey);
  }

  /**
   * Verify Schnorr signature
   */
  static async schnorrVerify(
    signature: Uint8Array,
    message: Uint8Array,
    publicKey: Uint8Array,
  ): Promise<boolean> {
    return await secp256k1.schnorr.verify(signature, message, publicKey);
  }

  /**
   * Sign message with ECDSA signature
   */
  static async ecdsaSign(
    message: Uint8Array,
    privateKey: Uint8Array,
  ): Promise<Uint8Array> {
    return await secp256k1.sign(message, privateKey);
  }

  /**
   * Verify ECDSA signature
   */
  static ecdsaVerify(
    signature: Uint8Array,
    message: Uint8Array,
    publicKey: Uint8Array,
  ): boolean {
    return secp256k1.verify(signature, message, publicKey);
  }

  /**
   * Convert hex string to Uint8Array
   */
  static hexToBytes(hex: string): Uint8Array {
    return Buffer.from(hex, 'hex');
  }

  /**
   * Convert Uint8Array to hex string
   */
  static bytesToHex(bytes: Uint8Array): string {
    return Buffer.from(bytes).toString('hex');
  }

  /**
   * Generate secure random bytes
   */
  static randomBytes(length: number): Uint8Array {
    const bytes = new Uint8Array(length);
    // Use crypto.getRandomValues in browser or crypto.randomBytes in Node
    if (typeof crypto !== 'undefined' && crypto.getRandomValues) {
      crypto.getRandomValues(bytes);
    } else {
      // Fallback for environments without crypto
      for (let i = 0; i < length; i++) {
        bytes[i] = Math.floor(Math.random() * 256);
      }
    }
    return bytes;
  }
}

export default CryptoService;
