/**
 * CryptoService Tests
 */

import {CryptoService} from '../src/services/crypto/CryptoService';

describe('CryptoService', () => {
  describe('generatePrivateKey', () => {
    it('generates a 32-byte private key', () => {
      const privateKey = CryptoService.generatePrivateKey();
      expect(privateKey).toHaveLength(32);
    });
  });

  describe('getPublicKey', () => {
    it('derives public key from private key', () => {
      const privateKey = CryptoService.generatePrivateKey();
      const publicKey = CryptoService.getPublicKey(privateKey);
      expect(publicKey).toBeTruthy();
      expect(publicKey.length).toBeGreaterThan(0);
    });
  });

  describe('sha256', () => {
    it('hashes string correctly', () => {
      const hash = CryptoService.sha256('test');
      expect(hash).toHaveLength(32);
    });

    it('produces consistent hashes', () => {
      const hash1 = CryptoService.sha256('test');
      const hash2 = CryptoService.sha256('test');
      expect(hash1).toEqual(hash2);
    });
  });

  describe('hexToBytes and bytesToHex', () => {
    it('converts between hex and bytes', () => {
      const hex = '1234567890abcdef';
      const bytes = CryptoService.hexToBytes(hex);
      const backToHex = CryptoService.bytesToHex(bytes);
      expect(backToHex).toBe(hex);
    });
  });

  describe('randomBytes', () => {
    it('generates random bytes of specified length', () => {
      const bytes = CryptoService.randomBytes(16);
      expect(bytes).toHaveLength(16);
    });
  });
});
