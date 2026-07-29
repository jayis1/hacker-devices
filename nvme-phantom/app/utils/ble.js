/**
 * utils/ble.js — BLE manager for NVMe-Phantom companion app
 *
 * Author:  jayis1
 * License: MIT
 *
 * Wraps react-native-ble-plx to connect to the NVMe-Phantom nRF52840 BLE
 * module, implement the framed + XTEA-CTR-encrypted command protocol, and
 * expose a clean async API to the screens.
 */

import { BleManager } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';

const NUS_SERVICE_UUID      = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_TX_CHAR_UUID      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write
const NUS_RX_CHAR_UUID      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify

const CMD_PING        = 0x01;
const CMD_GET_STATUS  = 0x02;
const CMD_SET_MODE    = 0x10;
const CMD_START_CAP   = 0x11;
const CMD_STOP_CAP    = 0x12;
const CMD_LOAD_RULES  = 0x20;
const CMD_CLEAR_RULES = 0x21;
const CMD_INJECT_CMD  = 0x30;
const CMD_OPAL_SEND   = 0x40;
const CMD_OPAL_RECV   = 0x41;
const CMD_SPOOF_IDENT = 0x50;
const CMD_DMA_ARM     = 0x60;
const CMD_DMA_GO      = 0x61;
const CMD_HOTPLUG     = 0x70;
const CMD_FW_DOWNLOAD = 0x80;
const CMD_FW_COMMIT   = 0x81;

const MODE_NAMES = [
  'Passive Tap', 'NVMe MITM', 'Opal Interrogation', 'Controller Spoof',
  'DMA Bridge', 'FW Extract', 'Hot-Plug Fault', 'Covert Exfil', 'Safe'
];

class BLEManager {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.connected = false;
    this.nonce = 0;
    this.rxBuffer = [];
    this.pendingResolve = null;
    this.key = [0x4A,0x41,0x59,0x49,0x53,0x31,0x4E,0x56,0x4D,0x50,0x48,0x41,0,0,0,0];
  }

  async requestPermissions() {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(v => v === PermissionsAndroid.RESULTS.GRANTED);
    }
    return true;
  }

  async scanAndConnect() {
    await this.requestPermissions();
    return new Promise((resolve, reject) => {
      this.manager.startDeviceScan(null, null, (error, device) => {
        if (error) { reject(error); return; }
        if (device.name && device.name.includes('NVMe-Phantom')) {
          this.manager.stopDeviceScan();
          this.connect(device).then(resolve).catch(reject);
        }
      });
      setTimeout(() => {
        this.manager.stopDeviceScan();
        reject(new Error('Scan timeout — device not found'));
      }, 10000);
    });
  }

  async connect(device) {
    this.device = await device.connect();
    await this.device.discoverAllServicesAndCharacteristics();
    const txChar = await this.device.readCharacteristicForService(
      NUS_SERVICE_UUID, NUS_TX_CHAR_UUID);
    // Subscribe to notifications on the RX characteristic
    await this.device.setupNotifications(
      await this.device.readCharacteristicForService(NUS_SERVICE_UUID, NUS_RX_CHAR_UUID));
    this.device.onCharacteristicValueChanged(NUS_RX_CHAR_UUID, (err, char) => {
      if (err || !char.value) return;
      const bytes = this.base64ToBytes(char.value);
      this._handleRx(bytes);
    });
    this.connected = true;
  }

  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.connected = false;
    }
  }

  isConnected() { return this.connected; }

  // ---- XTEA-CTR keystream (matches firmware ble_c2.c) ----

  _xteaEncryptBlock(v, key) {
    let sum = 0, delta = 0x9E3779B9;
    let v0 = v[0] | 0, v1 = v[1] | 0;
    let k = [key[0]|0, key[1]|0, key[2]|0, key[3]|0];
    for (let i = 0; i < 32; i++) {
      v0 = (v0 + (((v1 << 4 ^ (v1 >>> 5)) + v1) ^ (sum + k[sum & 3]))) | 0;
      sum = (sum + delta) | 0;
      v1 = (v1 + (((v0 << 4 ^ (v0 >>> 5)) + v0) ^ (sum + k[(sum >>> 11) & 3]))) | 0;
    }
    return [v0, v1];
  }

  _keystream(nonce, len) {
    const out = new Uint8Array(len);
    let counter = 0, i = 0;
    const k = [
      (this.key[0])|(this.key[1]<<8)|(this.key[2]<<16)|(this.key[3]<<24),
      (this.key[4])|(this.key[5]<<8)|(this.key[6]<<16)|(this.key[7]<<24),
      (this.key[8])|(this.key[9]<<8)|(this.key[10]<<16)|(this.key[11]<<24),
      (this.key[12])|(this.key[13]<<8)|(this.key[14]<<16)|(this.key[15]<<24),
    ];
    while (i < len) {
      const v = this._xteaEncryptBlock([nonce, counter], k);
      for (let b = 0; b < 8 && i < len; b++) {
        out[i++] = (v[b < 4 ? 0 : 1] >>> ((b % 4) * 8)) & 0xFF;
      }
      counter++;
    }
    return out;
  }

  _crc8(data, len) {
    let crc = 0;
    for (let i = 0; i < len; i++) {
      crc ^= data[i];
      for (let b = 0; b < 8; b++) {
        crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) & 0xFF : (crc << 1) & 0xFF;
      }
    }
    return crc;
  }

  // ---- Frame send / receive ----

  async sendCommand(cmd, payload = new Uint8Array(0)) {
    if (!this.connected) throw new Error('Not connected');
    const len = payload.length;
    const ks = this._keystream(this.nonce, len);
    const enc = new Uint8Array(len);
    for (let i = 0; i < len; i++) enc[i] = payload[i] ^ ks[i];

    const frame = new Uint8Array(5 + len + 2);
    frame[0] = 0xAA; frame[1] = 0x55;
    frame[2] = cmd; frame[3] = len & 0xFF; frame[4] = (len >> 8) & 0xFF;
    frame.set(enc, 5);
    const crc = this._crc8(frame.subarray(2, 5 + len), 3 + len);
    frame[5 + len] = crc; frame[5 + len + 1] = 0x0D;

    const txChar = await this.device.readCharacteristicForService(
      NUS_SERVICE_UUID, NUS_TX_CHAR_UUID);
    await txChar.writeWithResponse(this.bytesToBase64(frame));
    this.nonce++;

    // Wait for response
    return new Promise((resolve, reject) => {
      this.pendingResolve = (respCmd, respPayload) => {
        resolve({ cmd: respCmd, payload: respPayload });
      };
      setTimeout(() => {
        if (this.pendingResolve) {
          this.pendingResolve = null;
          reject(new Error('Response timeout'));
        }
      }, 5000);
    });
  }

  _handleRx(bytes) {
    // Accumulate into rxBuffer and parse frames
    this.rxBuffer.push(...bytes);
    while (this.rxBuffer.length >= 5) {
      if (this.rxBuffer[0] !== 0xAA || this.rxBuffer[1] !== 0x55) {
        this.rxBuffer.shift(); continue;
      }
      const plen = this.rxBuffer[3] | (this.rxBuffer[4] << 8);
      if (this.rxBuffer.length < 5 + plen + 2) break;
      const crc = this._crc8(this.rxBuffer.slice(2, 5 + plen), 3 + plen);
      if (crc !== this.rxBuffer[5 + plen]) {
        this.rxBuffer.splice(0, 5 + plen + 2); continue;
      }
      const cmd = this.rxBuffer[2];
      const ks = this._keystream(this.nonce, plen);
      const payload = new Uint8Array(plen);
      for (let i = 0; i < plen; i++) payload[i] = this.rxBuffer[5 + i] ^ ks[i];
      this.nonce++;
      this.rxBuffer.splice(0, 5 + plen + 2);
      if (this.pendingResolve) {
        const r = this.pendingResolve;
        this.pendingResolve = null;
        r(cmd, payload);
      }
    }
  }

  // ---- High-level command helpers ----

  async ping() {
    const r = await this.sendCommand(CMD_PING);
    return r.payload[0];
  }

  async getStatus() {
    const r = await this.sendCommand(CMD_GET_STATUS);
    const p = r.payload;
    return {
      mode: p[0],
      modeName: MODE_NAMES[p[0]] || 'Unknown',
      link: p[1],
      linkWidth: p[2],
      captureCount: p[3] | (p[4] << 8),
      sdFreeKB: p[5] | (p[6] << 8),
      batteryMV: p[7] | (p[8] << 8),
      charging: p[9] === 1,
    };
  }

  async setMode(mode) {
    return this.sendCommand(CMD_SET_MODE, new Uint8Array([mode]));
  }

  async startCapture(sessionId) {
    return this.sendCommand(CMD_START_CAP,
      new Uint8Array([sessionId & 0xFF, (sessionId >> 8) & 0xFF]));
  }

  async stopCapture() {
    const r = await this.sendCommand(CMD_STOP_CAP);
    const p = r.payload;
    return { records: p[0] | (p[1] << 8), durationS: p[2] | (p[3] << 8) };
  }

  async loadRule(rule32) {
    return this.sendCommand(CMD_LOAD_RULES, rule32);
  }

  async clearRules() { return this.sendCommand(CMD_CLEAR_RULES); }

  async injectCommand(sq64) { return this.sendCommand(CMD_INJECT_CMD, sq64); }

  async spoofIdent(ident4096) { return this.sendCommand(CMD_SPOOF_IDENT, ident4096); }

  async opalSend(password) {
    const pwBytes = new TextEncoder().encode(password);
    return this.sendCommand(CMD_OPAL_SEND, pwBytes);
  }

  async dmaArm(hostPhys, len, dir) {
    const p = new Uint8Array(13);
    for (let i = 0; i < 8; i++) p[i] = (hostPhys >> (i * 8)) & 0xFF;
    p[8] = len & 0xFF; p[9] = (len >> 8) & 0xFF;
    p[10] = (len >> 16) & 0xFF; p[11] = (len >> 24) & 0xFF;
    p[12] = dir;
    const r = await this.sendCommand(CMD_DMA_ARM, p);
    return r.payload[0] | (r.payload[1] << 8) |
           (r.payload[2] << 16) | (r.payload[3] << 24);
  }

  async dmaGo(token) {
    const p = new Uint8Array(4);
    p[0] = token & 0xFF; p[1] = (token >> 8) & 0xFF;
    p[2] = (token >> 16) & 0xFF; p[3] = (token >> 24) & 0xFF;
    return this.sendCommand(CMD_DMA_GO, p);
  }

  async hotplug(event) {
    return this.sendCommand(CMD_HOTPLUG, new Uint8Array([event]));
  }

  // ---- Base64 helpers ----

  bytesToBase64(bytes) {
    let bin = '';
    for (let i = 0; i < bytes.length; i++) bin += String.fromCharCode(bytes[i]);
    return btoa(bin);
  }

  base64ToBytes(b64) {
    const bin = atob(b64);
    const out = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
    return out;
  }
}

export default BLEManager;
export { MODE_NAMES };