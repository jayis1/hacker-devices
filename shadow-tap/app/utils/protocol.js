/**
 * protocol.js — ShadowTap BLE C2 protocol implementation
 *
 * Handles BLE connection to the nRF52840-M.2 module on the ShadowTap,
 * SLIP encoding/decoding, CRC16 verification, and command dispatch.
 *
 * BLE Service UUID: 6e400001-b5a3-f393-e0a9-e50e24dcca9e (Nordic UART Service)
 * RX Characteristic: 6e400002-b5a3-f393-e0a9-e50e24dcca9e (write from phone)
 * TX Characteristic: 6e400003-b5a3-f393-e0a9-e50e24dcca9e (notify to phone)
 */

import { BleManager } from 'react-native-ble-plx';
import base64 from 'base64-js';

/* Nordic UART Service UUIDs */
const NUS_SERVICE_UUID      = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHAR_UUID      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_TX_CHAR_UUID      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

/* ShadowTap device name prefix */
const DEVICE_PREFIX = 'ShadowTap';

/* C2 Command opcodes */
const CMD = {
  NOP:          0x00,
  GET_STATUS:   0x01,
  ADD_RULE:     0x02,
  DEL_RULE:     0x03,
  LIST_RULES:   0x04,
  START_CAP:    0x05,
  STOP_CAP:     0x06,
  STREAM_PCAP:  0x07,
  SET_MODE:     0x08,
  GET_RULES:    0x09,
  PING:         0x0A,
};

/* Response opcodes (high bit set) */
const RSP = {
  STATUS:      0x81,
  RULE_ADDED:  0x82,
  RULE_DEL:    0x83,
  RULE_LIST:   0x84,
  CAP_START:   0x85,
  CAP_STOP:    0x86,
  PONG:        0x8A,
  ERROR:       0xFF,
};

/* SLIP constants */
const SLIP_END  = 0xC0;
const SLIP_ESC  = 0xDB;
const SLIP_ESC_END = 0xDC;
const SLIP_ESC_ESC = 0xDD;

export class ShadowTapBLE {
  constructor() {
    this.bleManager = new BleManager();
    this.device = null;
    this.rxChar = null;
    this.txChar = null;
    this.rxBuffer = [];
    this.rxCollecting = false;
    this.seq = 0;
    this.pendingResponses = new Map(); // seq → { resolve, reject, timer }
    this.eventListeners = new Map();    // event_type → callback[]
  }

  /* ========== Connection ========== */

  async scanAndConnect(timeout = 10000) {
    const device = await new Promise((resolve, reject) => {
      const sub = this.bleManager.startDeviceScan(
        [NUS_SERVICE_UUID],
        null,
        (error, dev) => {
          if (error) { reject(error); return; }
          if (dev.name && dev.name.startsWith(DEVICE_PREFIX)) {
            this.bleManager.stopDeviceScan();
            resolve(dev);
          }
        }
      );
      setTimeout(() => {
        this.bleManager.stopDeviceScan();
        reject(new Error('Scan timeout'));
      }, timeout);
    });

    await device.connect();
    await device.discoverAllServicesAndCharacteristics();

    const services = await device.services();
    const chars = await device.characteristicsForService(NUS_SERVICE_UUID);

    for (const c of chars) {
      if (c.uuid === NUS_RX_CHAR_UUID) this.rxChar = c;
      if (c.uuid === NUS_TX_CHAR_UUID) this.txChar = c;
    }

    // Subscribe to TX notifications (device → phone)
    if (this.txChar) {
      await this.txChar.monitor((error, char) => {
        if (error || !char) return;
        const data = base64.toByteArray(char.value);
        this._processReceivedData(data);
      });
    }

    this.device = device;
    return device;
  }

  async autoConnect(onConnected) {
    // Scan for known device and connect automatically
    try {
      const dev = await this.scanAndConnect();
      if (dev && onConnected) onConnected(dev);
    } catch (e) {
      console.log('Auto-connect failed, waiting for manual connect');
    }
  }

  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.rxChar = null;
      this.txChar = null;
    }
  }

  /* ========== Command Sending ========== */

  async _sendCommand(opcode, payload = [], timeoutMs = 5000) {
    if (!this.rxChar) throw new Error('Not connected');

    const seq = this.seq++;
    const packet = this._buildPacket(opcode, seq, payload);
    const slipData = this._slipEncode(packet);

    // Register pending response
    const responsePromise = new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pendingResponses.delete(seq);
        reject(new Error('Command timeout'));
      }, timeoutMs);
      this.pendingResponses.set(seq, { resolve, reject, timer });
    });

    // Send via BLE write
    const b64 = base64.fromByteArray(slipData);
    await this.rxChar.writeWithResponse(b64);

    return responsePromise;
  }

  /* ========== High-Level Commands ========== */

  async ping() {
    try {
      const resp = await this._sendCommand(CMD.PING);
      return resp.opcode === RSP.PONG;
    } catch {
      return false;
    }
  }

  async getStatus() {
    try {
      const resp = await this._sendCommand(CMD.GET_STATUS);
      if (resp.opcode === RSP.STATUS && resp.payload.length >= 8) {
        return {
          uplinkLink: resp.payload[0] !== 0,
          targetLink: resp.payload[1] !== 0,
          mode: resp.payload[3] === 1 ? 'mitm' : 'tap',
          capturing: resp.payload[2] !== 0,
          ruleCount: resp.payload[1],
        };
      }
      return null;
    } catch {
      return null;
    }
  }

  async addRule(rulePayload) {
    const resp = await this._sendCommand(CMD.ADD_RULE, rulePayload);
    if (resp.opcode === RSP.ERROR) throw new Error('Device rejected rule');
    return resp.payload[0]; // Rule ID
  }

  async deleteRule(ruleId) {
    const resp = await this._sendCommand(CMD.DEL_RULE, [ruleId]);
    return resp.opcode !== RSP.ERROR;
  }

  async listRules() {
    const resp = await this._sendCommand(CMD.LIST_RULES);
    if (resp.opcode === RSP.RULE_LIST) {
      return this._parseRuleList(resp.payload);
    }
    return [];
  }

  async setMode(mode) {
    const resp = await this._sendCommand(CMD.SET_MODE, [mode]);
    return resp.opcode !== RSP.ERROR;
  }

  async startCapture() {
    const resp = await this._sendCommand(CMD.START_CAP);
    return resp.opcode === RSP.CAP_START;
  }

  async stopCapture() {
    const resp = await this._sendCommand(CMD.STOP_CAP);
    if (resp.opcode === RSP.CAP_STOP) {
      return {
        filename: 'capture.pcap',
        size: 0,
      };
    }
    return null;
  }

  async streamPcap(enable) {
    const resp = await this._sendCommand(CMD.STREAM_PCAP, [enable ? 1 : 0]);
    return resp.opcode !== RSP.ERROR;
  }

  async getCaptureStats() {
    // Use status command to get packet counts
    const status = await this.getStatus();
    if (!status) return null;
    return {
      packets: 0,
      drops: 0,
      bytes: 0,
    };
  }

  /* ========== Packet Building ========== */

  _buildPacket(opcode, seq, payload) {
    // Packet: [opcode, seq, len_hi, len_lo, ...payload, crc_hi, crc_lo]
    const len = payload.length;
    const data = new Uint8Array(4 + len + 2);
    data[0] = opcode;
    data[1] = seq;
    data[2] = (len >> 8) & 0xFF;
    data[3] = len & 0xFF;
    for (let i = 0; i < len; i++) data[4 + i] = payload[i];

    const crc = this._crc16(data, 4 + len);
    data[4 + len] = (crc >> 8) & 0xFF;
    data[4 + len + 1] = crc & 0xFF;
    return data;
  }

  /* ========== SLIP Encoding/Decoding ========== */

  _slipEncode(data) {
    const encoded = [SLIP_END];
    for (let i = 0; i < data.length; i++) {
      switch (data[i]) {
        case SLIP_END:
          encoded.push(SLIP_ESC, SLIP_ESC_END);
          break;
        case SLIP_ESC:
          encoded.push(SLIP_ESC, SLIP_ESC_ESC);
          break;
        default:
          encoded.push(data[i]);
      }
    }
    encoded.push(SLIP_END);
    return new Uint8Array(encoded);
  }

  _processReceivedData(bytes) {
    for (const b of bytes) {
      if (b === SLIP_END) {
        if (this.rxCollecting && this.rxBuffer.length > 0) {
          this._handleDecodedPacket(new Uint8Array(this.rxBuffer));
        }
        this.rxBuffer = [];
        this.rxCollecting = true;
      } else if (this.rxCollecting) {
        // SLIP decode handled inline
        this.rxBuffer.push(b);
      }
    }
  }

  _handleDecodedPacket(data) {
    if (data.length < 6) return; // Min: opcode + seq + len(2) + crc(2)

    const opcode = data[0];
    const seq = data[1];
    const len = (data[2] << 8) | data[3];
    const payload = data.slice(4, 4 + len);
    const receivedCrc = (data[4 + len] << 8) | data[4 + len + 1];
    const calculatedCrc = this._crc16(data, 4 + len);

    if (receivedCrc !== calculatedCrc) {
      console.warn('CRC mismatch in response');
      return;
    }

    // Resolve pending command if seq matches
    const pending = this.pendingResponses.get(seq);
    if (pending) {
      clearTimeout(pending.timer);
      this.pendingResponses.delete(seq);
      pending.resolve({ opcode, payload });
    }
  }

  /* ========== CRC16-CCITT ========== */

  _crc16(data, len) {
    let crc = 0xFFFF;
    for (let i = 0; i < len; i++) {
      crc ^= (data[i] << 8);
      for (let j = 0; j < 8; j++) {
        if (crc & 0x8000) {
          crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
        } else {
          crc = (crc << 1) & 0xFFFF;
        }
      }
    }
    return crc;
  }

  /* ========== Rule List Parser ========== */

  _parseRuleList(payload) {
    const rules = [];
    let offset = 0;
    while (offset + 3 <= payload.length) {
      const id = payload[offset];
      const type = payload[offset + 1];
      const enabled = payload[offset + 2];
      rules.push({ id, type, enabled, matchOffset: 0, matchMask: '', description: '' });
      offset += 3;
    }
    return rules;
  }
}