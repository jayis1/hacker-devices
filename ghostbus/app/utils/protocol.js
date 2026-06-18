/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — Protocol Utility
 *
 * BLE command framing, AES-GCM session key management, and protocol
 * helpers for the GHOSTBUS firmware interface.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import CryptoJS from 'crypto-js';
import { Buffer } from 'buffer';

/**
 * SoC IDCODE lookup table (subset of the firmware's database).
 * Maps DP/JTAG IDCODEs to vendor/family/exploit info.
 */
const SOC_DB = {
  '0x2BA01477': { vendor: 'ARM', family: 'CoreSight-DP (Cortex-M4)', exploit: null },
  '0x0BD11477': { vendor: 'ARM', family: 'CoreSight-DP v5 (Cortex-M7)', exploit: null },
  '0x6BA02477': { vendor: 'ARM', family: 'CoreSight-DP v6 (Cortex-M33)', exploit: null },
  '0x06400041': { vendor: 'STMicro', family: 'STM32F405', exploit: 'stm32f4_rdp1_downgrade' },
  '0x06418041': { vendor: 'STMicro', family: 'STM32F407', exploit: 'stm32f4_rdp1_downgrade' },
  '0x06B30041': { vendor: 'STMicro', family: 'STM32H7', exploit: null },
  '0x0C300440': { vendor: 'NXP', family: 'LPC55S6x', exploit: 'nxp_lpc_pwreplay' },
  '0x0BB11477': { vendor: 'Nordic', family: 'nRF52', exploit: null },
};

/**
 * GhostBusService — wraps BLE command sending with logging and
 * provides high-level methods for common operations.
 * Author: jayis1
 */
export class GhostBusService {
  constructor(sendCommand, log) {
    this.sendCommand = sendCommand;
    this.log = log;
    this.sessionKey = null;
  }

  /**
   * Discover the debug port on the connected target.
   */
  async discover() {
    this.log('Starting debug port discovery...');
    await this.sendCommand(0x01, Buffer.from([3]));
  }

  /**
   * Read memory from the target.
   */
  async readMemory(addr, len) {
    const payload = Buffer.alloc(8);
    payload.writeUInt32LE(addr, 0);
    payload.writeUInt32LE(len, 4);
    await this.sendCommand(0x0E, payload);
    this.log(`Reading 0x${addr.toString(16)} (${len} bytes)`);
  }

  /**
   * Extract firmware from target flash.
   */
  async extractFlash(addr, len) {
    const payload = Buffer.alloc(8);
    payload.writeUInt32LE(addr, 0);
    payload.writeUInt32LE(len, 4);
    await this.sendCommand(0x06, payload);
    this.log(`Extracting flash 0x${addr.toString(16)} (${len} bytes)`);
  }

  /**
   * Halt the target CPU core.
   */
  async halt() {
    await this.sendCommand(0x04, Buffer.alloc(0));
    this.log('Halting target core');
  }

  /**
   * Resume the target CPU core.
   */
  async resume() {
    await this.sendCommand(0x05, Buffer.alloc(0));
    this.log('Resuming target core');
  }

  /**
   * Query device status.
   */
  async status() {
    await this.sendCommand(0x0C, Buffer.alloc(0));
  }

  /**
   * Lock the device and disconnect BLE.
   */
  async lock() {
    await this.sendCommand(0x0D, Buffer.alloc(0));
    this.log('Locking device');
  }

  /**
   * Look up a SoC IDCODE in the local database.
   */
  lookupSoC(idcodeStr) {
    return SOC_DB[idcodeStr] || { vendor: 'Unknown', family: 'Unknown', exploit: null };
  }

  /**
   * Set the AES-256-GCM session key for decrypting BLE data.
   * Key is derived from ECDH P-256 at pairing.
   */
  setSessionKey(keyHex) {
    this.sessionKey = CryptoJS.enc.Hex.parse(keyHex);
    this.log('Session key established');
  }

  /**
   * Decrypt a BLE data chunk using AES-256-GCM.
   * @param {Buffer} ciphertext - encrypted payload
   * @param {Buffer} nonce - 12-byte nonce
   * @param {Buffer} tag - 16-byte GCM tag
   * @returns {Buffer} plaintext
   */
  decryptChunk(ciphertext, nonce, tag) {
    if (!this.sessionKey) throw new Error('No session key');
    const key = this.sessionKey;
    const nonceWord = CryptoJS.lib.WordArray.create(nonce);
    // AES-GCM decrypt (simplified — in production use a native GCM lib)
    const cipherParams = CryptoJS.lib.CipherParams.create({
      ciphertext: CryptoJS.lib.WordArray.create(ciphertext),
    });
    try {
      const plain = CryptoJS.AES.decrypt(cipherParams, key, {
        iv: nonceWord,
        mode: CryptoJS.mode.GCM,
      });
      return Buffer.from(plain.toString(CryptoJS.enc.Latin1), 'latin1');
    } catch (e) {
      this.log(`Decryption failed: ${e.message}`, 'error');
      return Buffer.alloc(0);
    }
  }

  /**
   * Verify SHA-256 integrity of an extracted flash block.
   */
  verifyHash(data, expectedHashHex) {
    const computed = CryptoJS.SHA256(CryptoJS.lib.WordArray.create(data)).toString();
    return computed === expectedHashHex;
  }

  /**
   * Parse a discovery result frame from the device.
   * Frame layout (16 bytes):
   *   [0] protocol, [1-4] idcode LE, [5] swdio_ch, [6] swclk_ch,
   *   [7] tck_ch, [8] tms_ch, [9] tdi_ch, [10] tdo_ch,
   *   [11] gnd_ch, [12] vref_ch, [13-14] vref_mv LE, [15] confidence
   */
  parseDiscoveryFrame(frame) {
    if (!frame || frame.length < 16) return null;
    const protos = ['UNKNOWN', 'SWD', 'JTAG', 'CJTAG', 'POWER'];
    const idcode = frame.readUInt32LE(1);
    return {
      protocol: protos[frame[0]] || 'UNKNOWN',
      idcode: '0x' + idcode.toString(16).toUpperCase().padStart(8, '0'),
      swdio_ch: frame[5],
      swclk_ch: frame[6],
      tck_ch: frame[7],
      tms_ch: frame[8],
      tdi_ch: frame[9],
      tdo_ch: frame[10],
      gnd_ch: frame[11],
      vref_ch: frame[12],
      vref_mv: frame.readUInt16LE(13),
      confidence: frame[15],
      soc: this.lookupSoC('0x' + idcode.toString(16).toUpperCase().padStart(8, '0')),
    };
  }

  /**
   * Parse a status frame (8 bytes):
   *   [0] state, [1] protocol, [2] battery%, [3-6] dump_progress LE, [7] ble_connected
   */
  parseStatusFrame(frame) {
    if (!frame || frame.length < 8) return null;
    const states = ['IDLE', 'DISCOVERY', 'SWD_CONNECTED', 'JTAG_CONNECTED',
                    'FLASH_DUMP', 'LOCKED'];
    return {
      state: states[frame[0]] || 'UNKNOWN',
      protocol: frame[1],
      battery: frame[2],
      dumpProgress: frame.readUInt32LE(3),
      bleConnected: frame[7] === 1,
    };
  }
}

/**
 * Build a flash-extraction request payload.
 */
export function buildExtractPayload(addr, len) {
  const buf = Buffer.alloc(8);
  buf.writeUInt32LE(addr, 0);
  buf.writeUInt32LE(len, 4);
  return buf;
}

/**
 * Format an address for display.
 */
export function formatAddr(addr) {
  return '0x' + addr.toString(16).toUpperCase().padStart(8, '0');
}

/**
 * Format bytes as a hex string.
 */
export function toHex(buf, separator = ' ') {
  return Array.from(buf).map(b => b.toString(16).padStart(2, '0')).join(separator);
}