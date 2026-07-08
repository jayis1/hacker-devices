/*
 * tpm-phantom — app/utils/bleManager.js
 * BLE connection manager for the tpm-phantom device.
 *
 * Uses react-native-ble-plx to connect to the nRF52840 and exchange
 * wire-protocol frames over the Nordic UART Service (NUS).
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import { BleManager } from 'react-native-ble-plx';
import { FrameParser, packFrame } from './protocol';

// Nordic UART Service UUIDs
const NUS_SERVICE       = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHAR       = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write to device
const NUS_TX_CHAR       = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify from device

const DEVICE_NAME_PREFIX = 'tpm-phantom';

class BleConnection {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.parser = null;
    this.onFrame = null;
    this.onStatus = null;
    this.connected = false;
    this.scanning = false;
  }

  // =============================================================
  // Scan for tpm-phantom devices
  // =============================================================
  scan(onFound, durationMs = 10000) {
    if (this.scanning) return;
    this.scanning = true;

    this.manager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        this.scanning = false;
        if (this.onStatus) this.onStatus('error', error.message);
        return;
      }
      if (device.name && device.name.startsWith(DEVICE_NAME_PREFIX)) {
        this.manager.stopDeviceScan();
        this.scanning = false;
        onFound(device);
      }
    });

    setTimeout(() => {
      if (this.scanning) {
        this.manager.stopDeviceScan();
        this.scanning = false;
      }
    }, durationMs);
  }

  // =============================================================
  // Connect to a discovered device
  // =============================================================
  async connect(device) {
    try {
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();

      // Set up notification handler on NUS TX characteristic
      const txChar = await this.device.readCharacteristicForService(
        NUS_SERVICE, NUS_TX_CHAR
      );

      this.parser = new FrameParser((cmd, payload) => {
        if (this.onFrame) this.onFrame(cmd, payload);
      });

      txChar.monitor((error, characteristic) => {
        if (error) {
          if (this.onStatus) this.onStatus('error', error.message);
          return;
        }
        if (characteristic.value) {
          // Decode base64 to bytes
          const raw = base64ToByteArray(characteristic.value);
          this.parser.feed(raw);
        }
      });

      this.connected = true;
      if (this.onStatus) this.onStatus('connected', this.device.name);
      return true;
    } catch (e) {
      if (this.onStatus) this.onStatus('error', e.message);
      return false;
    }
  }

  // =============================================================
  // Send a command frame over NUS RX characteristic
  // =============================================================
  async send(cmd, payload) {
    if (!this.connected || !this.device) return false;
    const frame = packFrame(cmd, payload);
    const b64 = byteArrayToBase64(frame);
    try {
      await this.device.writeCharacteristicWithResponseForService(
        NUS_SERVICE, NUS_RX_CHAR, b64
      );
      return true;
    } catch (e) {
      if (this.onStatus) this.onStatus('error', e.message);
      return false;
    }
  }

  // =============================================================
  // Disconnect
  // =============================================================
  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.connected = false;
      if (this.onStatus) this.onStatus('disconnected', '');
    }
  }

  isConnected() {
    return this.connected;
  }
}

// =============================================================
// Base64 helpers (react-native-ble-plx uses base64 strings)
// =============================================================
function byteArrayToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  let i = 0;
  while (i < bytes.length) {
    const a = bytes[i++] || 0;
    const b = bytes[i++] || 0;
    const c = bytes[i++] || 0;
    const triplet = (a << 16) | (b << 8) | c;
    result += chars[(triplet >> 18) & 0x3f];
    result += chars[(triplet >> 12) & 0x3f];
    result += i - 2 < bytes.length ? chars[(triplet >> 6) & 0x3f] : '=';
    result += i - 1 < bytes.length ? chars[triplet & 0x3f] : '=';
  }
  return result;
}

function base64ToByteArray(str) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const lookup = new Uint8Array(128);
  for (let i = 0; i < chars.length; i++) lookup[chars.charCodeAt(i)] = i;
  const len = str.length;
  const outLen = (len * 3) >> 2;
  const out = new Uint8Array(outLen);
  let p = 0;
  for (let i = 0; i < len; i += 4) {
    const a = lookup[str.charCodeAt(i)] || 0;
    const b = lookup[str.charCodeAt(i + 1)] || 0;
    const c = lookup[str.charCodeAt(i + 2)] || 0;
    const d = lookup[str.charCodeAt(i + 3)] || 0;
    const triplet = (a << 18) | (b << 12) | (c << 6) | d;
    if (p < outLen) out[p++] = (triplet >> 16) & 0xff;
    if (p < outLen) out[p++] = (triplet >> 8) & 0xff;
    if (p < outLen) out[p++] = triplet & 0xff;
  }
  return out;
}

export default new BleConnection();