/**
 * ACOUSTIC-PHANTOM — AES-256-CTR decryption utility
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Decrypts AES-256-CTR encrypted audio captures stored on the
 * device's microSD card. Uses the aes-js library for the crypto
 * operations. The key is derived from the BLE session key via
 * ECDH and shared securely during pairing.
 */

import aesjs from 'aes-js';

/**
 * Decrypt an AES-256-CTR encrypted capture file.
 * @param {Uint8Array} encryptedData - The encrypted file bytes
 * @param {Uint8Array} key - 32-byte AES-256 key
 * @param {Uint8Array} iv - 16-byte initial counter value
 * @returns {Uint8Array} Decrypted plaintext
 */
export function decryptCapture(encryptedData, key, iv) {
  if (key.length !== 32) {
    throw new Error('AES-256 key must be 32 bytes');
  }
  if (iv.length !== 16) {
    throw new Error('IV must be 16 bytes');
  }

  // AES-256-CTR: the IV is the initial counter value
  const aesCtr = new aesjs.ModeOfOperation.ctr(key, new aesjs.Counter(iv));
  const decrypted = aesCtr.decrypt(encryptedData);

  return new Uint8Array(decrypted);
}

/**
 * Derive a pseudo-random 32-byte key from a 6-character BLE passkey.
 * In production, this would use ECDH-derived shared secret. For the
 * companion app's demo mode, we use a simple hash expansion.
 * @param {string} passkey - 6-digit BLE pairing passkey
 * @returns {Uint8Array} 32-byte key
 */
export function deriveKeyFromPasskey(passkey) {
  // Simple key derivation: expand passkey to 32 bytes using
  // repeated hashing. NOT secure for production — use ECDH.
  const key = new Uint8Array(32);
  const passkeyBytes = new TextEncoder().encode(passkey);

  // Simple expansion: fill key with passkey bytes, then XOR with
  // a fixed salt. Real implementation uses HKDF or PBKDF2.
  for (let i = 0; i < 32; i++) {
    key[i] = passkeyBytes[i % passkeyBytes.length] ^ (i * 7 + 0x37);
  }

  return key;
}

/**
 * Parse a WAV file header from decrypted data.
 * @param {Uint8Array} data - Decrypted WAV file bytes
 * @returns {object} Parsed header info: { sampleRate, channels, bitsPerSample, dataSize }
 */
export function parseWavHeader(data) {
  if (data.length < 44) return null;

  // Check RIFF/WAVE magic
  const riff = String.fromCharCode(data[0], data[1], data[2], data[3]);
  const wave = String.fromCharCode(data[8], data[9], data[10], data[11]);
  if (riff !== 'RIFF' || wave !== 'WAVE') return null;

  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);

  return {
    sampleRate: view.getUint32(24, true),
    channels: view.getUint16(22, true),
    bitsPerSample: view.getUint16(34, true),
    dataSize: view.getUint32(40, true),
    audioStart: 44,
  };
}

/**
 * Extract 16-bit PCM samples from a WAV file.
 * @param {Uint8Array} data - Decrypted WAV file bytes
 * @returns {Int16Array} Audio samples
 */
export function extractPcmSamples(data) {
  const header = parseWavHeader(data);
  if (!header) return null;

  const numSamples = Math.floor(header.dataSize / 2);
  const samples = new Int16Array(numSamples);
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);

  for (let i = 0; i < numSamples; i++) {
    samples[i] = view.getInt16(header.audioStart + i * 2, true);
  }

  return samples;
}