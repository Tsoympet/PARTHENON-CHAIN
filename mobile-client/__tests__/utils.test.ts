/**
 * Utility Function Tests
 */

import {
  formatCrypto,
  formatFiat,
  truncateAddress,
  isValidAddress,
  isValidAmount,
} from '../src/utils';

describe('Format Utils', () => {
  describe('formatCrypto', () => {
    it('formats crypto amount correctly', () => {
      expect(formatCrypto(100.12345678)).toBe('100.12345678 DRACHMA');
      expect(formatCrypto(0.5, 2, 'BTC')).toBe('0.50 BTC');
    });
  });

  describe('formatFiat', () => {
    it('formats fiat currency correctly', () => {
      expect(formatFiat(100.5)).toBe('$100.50');
      expect(formatFiat(1234.56)).toBe('$1,234.56');
    });
  });

  describe('truncateAddress', () => {
    it('truncates long addresses', () => {
      const address = 'drm1234567890abcdefghijklmnopqrstuvwxyz';
      const truncated = truncateAddress(address);
      expect(truncated).toBe('drm1234567...rstuvwxyz');
    });

    it('does not truncate short addresses', () => {
      const address = 'drm123';
      expect(truncateAddress(address)).toBe(address);
    });
  });
});

describe('Validation Utils', () => {
  describe('isValidAddress', () => {
    it('validates correct addresses', () => {
      expect(isValidAddress('drm1234567890abcdef1234567890abcdef12345678')).toBe(true);
    });

    it('rejects invalid addresses', () => {
      expect(isValidAddress('invalid')).toBe(false);
      expect(isValidAddress('0x1234')).toBe(false);
      expect(isValidAddress('drm123')).toBe(false);
    });
  });

  describe('isValidAmount', () => {
    it('validates correct amounts', () => {
      expect(isValidAmount('100')).toBe(true);
      expect(isValidAmount('0.5')).toBe(true);
    });

    it('rejects invalid amounts', () => {
      expect(isValidAmount('0')).toBe(false);
      expect(isValidAmount('-10')).toBe(false);
      expect(isValidAmount('abc')).toBe(false);
    });
  });
});
