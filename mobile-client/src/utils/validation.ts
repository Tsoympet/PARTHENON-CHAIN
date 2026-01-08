/**
 * Validation Utilities
 */

/**
 * Validate Drachma address format
 */
export const isValidAddress = (address: string): boolean => {
  // Drachma addresses start with 'drm' followed by hex characters
  const addressRegex = /^drm[0-9a-f]{40,}$/i;
  return addressRegex.test(address);
};

/**
 * Validate amount
 */
export const isValidAmount = (amount: string): boolean => {
  const num = parseFloat(amount);
  return !isNaN(num) && num > 0 && isFinite(num);
};

/**
 * Validate mnemonic phrase
 */
export const isValidMnemonic = (mnemonic: string): boolean => {
  const words = mnemonic.trim().split(/\s+/);
  // BIP39 standard supports 12, 15, 18, 21, or 24 words
  return [12, 15, 18, 21, 24].includes(words.length);
};

/**
 * Validate private key format
 */
export const isValidPrivateKey = (key: string): boolean => {
  const hexRegex = /^[0-9a-f]{64}$/i;
  return hexRegex.test(key);
};

/**
 * Sanitize input to prevent injection
 */
export const sanitizeInput = (input: string): string => {
  return input.replace(/[<>]/g, '');
};
