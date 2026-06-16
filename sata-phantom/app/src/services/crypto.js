/**
 * crypto.js — Client-side Encryption Helpers
 * Author: jayis1
 */

/**
 * Convert a hex string to a Uint8Array.
 */
export const hexToBytes = (hex) => {
  const clean = hex.replace(/\s+/g, '');
  const bytes = new Uint8Array(clean.length / 2);
  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(clean.substr(i * 2, 2), 16);
  }
  return bytes;
};

/**
 * Convert a byte array to hex string.
 */
export const bytesToHex = (bytes) => {
  return Array.from(bytes)
    .map(b => b.toString(16).padStart(2, '0'))
    .join('');
};

/**
 * Simple XOR obfuscation (mirrors the firmware's UDP XOR transport).
 */
export const xorObfuscate = (data, key) => {
  const result = new Uint8Array(data.length);
  let k = key & 0xFF;
  for (let i = 0; i < data.length; i++) {
    result[i] = data[i] ^ k;
    k = (k * 0x1B + 0x3D) & 0xFF;
  }
  return result;
};

/**
 * Verify GCM auth tag (placeholder — real implementation uses Web Crypto API).
 */
export const verifyAuthTag = async (ciphertext, key) => {
  // In production, use SubtleCrypto API:
  // const key_ = await crypto.subtle.importKey('raw', key, 'AES-GCM', false, ['decrypt']);
  // const decrypted = await crypto.subtle.decrypt({ name: 'AES-GCM', iv: ciphertext.slice(0,12) }, key_, ciphertext.slice(12));
  // return decrypted;
  return true;  // Placeholder
};

/**
 * Generate a random session key.
 */
export const generateSessionKey = () => {
  const key = new Uint8Array(32);
  crypto.getRandomValues(key);
  return key;
};

export default { hexToBytes, bytesToHex, xorObfuscate, verifyAuthTag, generateSessionKey };
