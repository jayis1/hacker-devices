/**
 * protocol.js — FiberPhantom BLE C2 Protocol
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Implements the BLE GATT communication protocol between the companion app
 * and the FiberPhantom device (via the nRF52840 BLE module).
 *
 * Protocol: TLV (Type-Length-Value) framed over BLE notifications.
 * Frame format: [SOH(0x01)] [TYPE] [SEQ] [FLAGS] [LEN] [payload...] [CRC8] [EOT(0x04)]
 */

import base64 from 'base64-js';

// ---- BLE UUIDs ----
export const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
export const TX_CHAR_UUID  = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Write (app→dev)
export const RX_CHAR_UUID  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Notify (dev→app)

// ---- C2 message types (must match firmware ble_c2_driver.h) ----
export const C2_MSG_TYPE = {
  PING:           0x01,
  PONG:           0x02,
  GET_STATUS:     0x03,
  STATUS:         0x04,
  GET_STATS:      0x05,
  STATS:          0x06,
  SET_MODE:       0x07,
  ACK:            0x08,
  NACK:           0x09,
  SET_APD_BIAS:   0x0A,
  START_CAPTURE:  0x0B,
  STOP_CAPTURE:   0x0C,
  SET_RULE:       0x0D,
  CLEAR_RULE:     0x0E,
  CLEAR_ALL_RULES: 0x0F,
  ENABLE_MITM:    0x10,
  ENABLE_INJECT:  0x11,
  GET_RULE:       0x12,
  RULE_DATA:      0x13,
  AUTO_CALIBRATE: 0x14,
  CALIB_RESULT:   0x15,
  GET_BATT:       0x16,
  BATT:           0x17,
  SET_AGC:        0x18,
  FRAME_NOTIFY:   0x20,
  ALERT:          0x21,
  GET_VERSION:    0x30,
  VERSION:        0x31,
  DOWNLOAD_PCAP:  0x40,
  PCAP_CHUNK:     0x41,
  PCAP_END:       0x42,
  LIST_FILES:     0x50,
  FILE_ENTRY:     0x51,
  FILE_LIST_END:  0x52,
  DELETE_FILE:    0x53,
};

// ---- Frame constants ----
const SOH = 0x01;
const EOT = 0x04;
const BLE_CHUNK_SIZE = 20;
const MAX_PAYLOAD = 244;

// ---- MITM rule types ----
export const MITM_RULE_TYPE = {
  PASS:          0,
  DROP:          1,
  MODIFY:        2,
  INJECT_AFTER:  3,
  INJECT_BEFORE: 4,
  DELAY:         5,
};

// ---- CRC-8 (polynomial 0x07) ----
function crc8(data) {
  let crc = 0x00;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ 0x07) & 0xFF;
      } else {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  return crc;
}

// ---- FiberPhantomBLE class ----
export class FiberPhantomBLE {
  constructor(bleManager) {
    this.bleManager = bleManager;
    this.device = null;
    this.txCharacteristic = null;
    this.rxCharacteristic = null;
    this.seq = 0;
    this.rxBuffer = [];
    this.rxAssembler = [];
    this.connected = false;
  }

  static SERVICE_UUID = SERVICE_UUID;
  static RX_CHAR_UUID = RX_CHAR_UUID;
  static TX_CHAR_UUID = TX_CHAR_UUID;

  // ---- Connect to device ----
  async connect(device) {
    this.device = device;
    await device.connect({ requestMTU: 247 });
    await device.discoverAllServicesAndCharacteristics();

    const characteristics = await device.characteristicsForService(SERVICE_UUID);
    this.txCharacteristic = characteristics.find(c => c.uuid === TX_CHAR_UUID);
    this.rxCharacteristic = characteristics.find(c => c.uuid === RX_CHAR_UUID);

    this.connected = true;
    return device;
  }

  // ---- Disconnect ----
  async disconnect() {
    if (this.device && this.connected) {
      await this.device.cancelConnection();
      this.connected = false;
    }
  }

  // ---- Send a C2 command ----
  async sendCommand(type, payload = []) {
    if (!this.connected || !this.txCharacteristic) {
      throw new Error('Not connected to device');
    }

    const seq = this.seq++ & 0xFF;
    const flags = 0x01; // CRC present
    const length = Math.min(payload.length, MAX_PAYLOAD);

    // Build frame: [SOH] [TYPE] [SEQ] [FLAGS] [LEN] [payload] [CRC8] [EOT]
    const frame = [SOH, type, seq, flags, length, ...payload.slice(0, length)];

    // Calculate CRC over type, seq, flags, length, payload
    const crcData = frame.slice(1); // Everything after SOH
    const crc = crc8(crcData);
    frame.push(crc, EOT);

    // Split into BLE chunks (20 bytes each) and write
    for (let i = 0; i < frame.length; i += BLE_CHUNK_SIZE) {
      const chunk = frame.slice(i, Math.min(i + BLE_CHUNK_SIZE, frame.length));
      const base64Data = base64.fromByteArray(new Uint8Array(chunk));
      await this.txCharacteristic.writeWithResponse(base64Data);
    }

    return seq;
  }

  // ---- Process a BLE notification (base64-encoded) ----
  // Returns an array of parsed C2 messages
  processNotification(base64Value) {
    const bytes = base64.toByteArray(base64Value);
    this.rxAssembler.push(...bytes);

    const messages = [];

    // Look for complete frames in the assembly buffer
    while (this.rxAssembler.length >= 7) { // Minimum frame: SOH+TYPE+SEQ+FLAGS+LEN+CRC+EOT = 7 bytes
      // Find SOH
      let sohIdx = this.rxAssembler.indexOf(SOH);
      if (sohIdx === -1) {
        this.rxAssembler = [];
        break;
      }
      if (sohIdx > 0) {
        this.rxAssembler = this.rxAssembler.slice(sohIdx);
      }

      if (this.rxAssembler.length < 7) break;

      const length = this.rxAssembler[4];
      const frameEnd = 5 + length + 2; // payload + CRC + EOT

      if (this.rxAssembler.length < frameEnd) break;

      // Check EOT
      if (this.rxAssembler[frameEnd - 1] !== EOT) {
        // Invalid frame, discard SOH and try again
        this.rxAssembler.shift();
        continue;
      }

      // Extract frame
      const frame = this.rxAssembler.slice(0, frameEnd);

      // Validate CRC
      const crcData = frame.slice(1, 5 + length); // type, seq, flags, length, payload
      const expectedCRC = crc8(crcData);
      const receivedCRC = frame[5 + length];

      if (expectedCRC === receivedCRC) {
        messages.push({
          type: frame[1],
          seq: frame[2],
          flags: frame[3],
          length: length,
          payload: Array.from(frame.slice(5, 5 + length)),
          crc: receivedCRC,
        });
      }

      // Remove processed frame from buffer
      this.rxAssembler = this.rxAssembler.slice(frameEnd);
    }

    return messages;
  }

  // ---- Convenience methods for common commands ----

  async ping() {
    return this.sendCommand(C2_MSG_TYPE.PING);
  }

  async getStatus() {
    return this.sendCommand(C2_MSG_TYPE.GET_STATUS);
  }

  async getStats() {
    return this.sendCommand(C2_MSG_TYPE.GET_STATS);
  }

  async setMode(mode) {
    return this.sendCommand(C2_MSG_TYPE.SET_MODE, [mode]);
  }

  async setApdBias(biasMV) {
    const payload = [
      (biasMV >> 24) & 0xFF,
      (biasMV >> 16) & 0xFF,
      (biasMV >> 8) & 0xFF,
      biasMV & 0xFF,
    ];
    return this.sendCommand(C2_MSG_TYPE.SET_APD_BIAS, payload);
  }

  async startCapture() {
    return this.sendCommand(C2_MSG_TYPE.START_CAPTURE);
  }

  async stopCapture() {
    return this.sendCommand(C2_MSG_TYPE.STOP_CAPTURE);
  }

  async setRule(index, rule) {
    // Pack mitm_rule_t (108 bytes) into payload
    const ruleBytes = packRule(rule);
    const payload = [index, ...ruleBytes];
    return this.sendCommand(C2_MSG_TYPE.SET_RULE, payload);
  }

  async clearRule(index) {
    return this.sendCommand(C2_MSG_TYPE.CLEAR_RULE, [index]);
  }

  async clearAllRules() {
    return this.sendCommand(C2_MSG_TYPE.CLEAR_ALL_RULES);
  }

  async enableMitm(enable) {
    return this.sendCommand(C2_MSG_TYPE.ENABLE_MITM, [enable ? 1 : 0]);
  }

  async enableInject(enable) {
    return this.sendCommand(C2_MSG_TYPE.ENABLE_INJECT, [enable ? 1 : 0]);
  }

  async autoCalibrate() {
    return this.sendCommand(C2_MSG_TYPE.AUTO_CALIBRATE);
  }

  async getBattery() {
    return this.sendCommand(C2_MSG_TYPE.GET_BATT);
  }

  async setAgc(enable) {
    return this.sendCommand(C2_MSG_TYPE.SET_AGC, [enable ? 1 : 0]);
  }

  async getVersion() {
    return this.sendCommand(C2_MSG_TYPE.GET_VERSION);
  }

  async listFiles() {
    return this.sendCommand(C2_MSG_TYPE.LIST_FILES);
  }

  async downloadPcap(filename) {
    const payload = [];
    for (let i = 0; i < filename.length && i < 32; i++) {
      payload.push(filename.charCodeAt(i));
    }
    return this.sendCommand(C2_MSG_TYPE.DOWNLOAD_PCAP, payload);
  }
}

// ---- Pack a MITM rule into 108 bytes ----
export function packRule(rule) {
  const bytes = new Uint8Array(108);
  let offset = 0;

  bytes[offset++] = rule.type || 0;
  bytes[offset++] = rule.enabled ? 1 : 0;
  bytes[offset++] = (rule.matchOffset >> 8) & 0xFF;
  bytes[offset++] = rule.matchOffset & 0xFF;
  bytes[offset++] = (rule.matchLength >> 8) & 0xFF;
  bytes[offset++] = rule.matchLength & 0xFF;

  // matchData[32]
  for (let i = 0; i < 32; i++) {
    bytes[offset++] = rule.matchData ? (rule.matchData[i] || 0) : 0;
  }

  // matchMask[32]
  for (let i = 0; i < 32; i++) {
    bytes[offset++] = rule.matchMask ? (rule.matchMask[i] || 0) : 0;
  }

  bytes[offset++] = (rule.replaceOffset >> 8) & 0xFF;
  bytes[offset++] = rule.replaceOffset & 0xFF;

  // replaceData[32]
  for (let i = 0; i < 32; i++) {
    bytes[offset++] = rule.replaceData ? (rule.replaceData[i] || 0) : 0;
  }

  bytes[offset++] = (rule.replaceLength >> 8) & 0xFF;
  bytes[offset++] = rule.replaceLength & 0xFF;
  bytes[offset++] = (rule.delayUs >> 8) & 0xFF;
  bytes[offset++] = rule.delayUs & 0xFF;

  return Array.from(bytes);
}

// ---- Parse a hex string to byte array ----
export function hexToBytes(hex) {
  const bytes = [];
  for (let i = 0; i < hex.length; i += 2) {
    bytes.push(parseInt(hex.substr(i, 2), 16));
  }
  return bytes;
}

// ---- Format bytes as hex string ----
export function bytesToHex(bytes) {
  return bytes.map(b => b.toString(16).padStart(2, '0')).join(' ');
}

// ---- Format byte count as human-readable ----
export function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
}

// ---- Format uptime ----
export function formatUptime(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (h > 0) return `${h}h ${m}m ${s}s`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}