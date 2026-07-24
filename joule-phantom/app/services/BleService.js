/**
 * BleService.js — BLE communication layer for Joule-Phantom app.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Wraps react-native-ble-plx to provide a simple event-based API for
 * talking to the Joule-Phantom hardware.  Implements the same framing
 * protocol as the firmware ble_link module: [SOF][LEN][OP][payload][CRC8][EOF].
 */

import { BleManager } from 'react-native-ble-plx';
import base64 from 'react-native-base64';

// Opcodes (must match firmware ble_link.h)
export const OP = {
  HELLO:          0x01,
  STATUS:         0x02,
  FRAME:          0x10,
  RULE_ADD:       0x20,
  RULE_CLR:       0x21,
  RULE_LIST:      0x22,
  RULE_LIST_RSP:  0x23,
  INJECT:         0x30,
  GLITCH:         0x40,
  SET_MODE:       0x50,
  AUTH_BYPASS:    0x60,
  ACK:            0xF0,
  NACK:           0xF1,
};

// Operating modes (must match firmware)
export const MODE = {
  PASSTHROUGH: 0,
  MITM:        1,
  JAM:         2,
  SPOOF_FULL:  3,
  STANDBY:     4,
};

const SOF = 0xAA;
const EOF = 0x55;

// SBS command name lookup
export const SBS_NAMES = {
  0x00: 'ManufacturerAccess',  0x01: 'AtRate',
  0x05: 'Temperature',          0x06: 'Voltage',
  0x07: 'Current',              0x08: 'AverageCurrent',
  0x0D: 'RelativeSoC',          0x0E: 'AbsoluteSoC',
  0x0F: 'RemainingCapacity',    0x10: 'FullChargeCapacity',
  0x11: 'RunTimeToEmpty',       0x12: 'AvgTimeToEmpty',
  0x13: 'AvgTimeToFull',        0x14: 'ChargingCurrent',
  0x15: 'ChargingVoltage',      0x16: 'BatteryStatus',
  0x17: 'CycleCount',           0x18: 'DesignCapacity',
  0x1B: 'ManufactureDate',      0x1C: 'SerialNumber',
  0x20: 'ManufacturerName',     0x21: 'DeviceName',
  0x22: 'DeviceChemistry',      0x23: 'ManufacturerData',
};

class JouleBleService {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.txChar = null;
    this.rxChar = null;
    this.serviceUuid = null;
    this.txUuid = null;
    this.rxUuid = null;

    this._statusCb = null;
    this._frameCb = null;
    this._rulesCb = null;
    this._connCb = null;

    this._rxBuffer = [];
    this._rxState = 0;
    this._rxLen = 0;
    this._rxOp = 0;
    this._rxPayload = [];
    this._rxCrc = 0;
  }

  initialize(serviceUuid, txUuid, rxUuid) {
    this.serviceUuid = serviceUuid;
    this.txUuid = txUuid;
    this.rxUuid = rxUuid;
  }

  onStatus(cb) { this._statusCb = cb; return () => { this._statusCb = null; }; }
  onFrame(cb)  { this._frameCb = cb;  return () => { this._frameCb = null; }; }
  onRules(cb)  { this._rulesCb = cb;  return () => { this._rulesCb = null; }; }
  onConnection(cb) { this._connCb = cb; return () => { this._connCb = null; }; }

  async connect() {
    if (!this.serviceUuid) throw new Error('Not initialized');

    this.manager.startDeviceScan(null, null, (error, device) => {
      if (error) { console.log('Scan error:', error); return; }
      if (device.name && device.name.includes('Joule')) {
        this.manager.stopDeviceScan();
        this._connectToDevice(device);
      }
    });
  }

  async _connectToDevice(device) {
    try {
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();
      const services = await this.device.services();

      for (const svc of services) {
        if (svc.uuid.toLowerCase() === this.serviceUuid) {
          const chars = await svc.characteristics();
          for (const c of chars) {
            if (c.uuid.toLowerCase() === this.txUuid) this.txChar = c;
            if (c.uuid.toLowerCase() === this.rxUuid) this.rxChar = c;
          }
        }
      }

      if (this.rxChar) {
        this.rxChar.monitor((err, char) => {
          if (err) { console.log('Monitor error:', err); return; }
          if (char && char.value) {
            const bytes = base64.decode(char.value);
            for (let i = 0; i < bytes.length; i++) {
              this._rxByte(bytes.charCodeAt(i));
            }
          }
        });
      }

      if (this._connCb) this._connCb(true);
    } catch (e) {
      console.log('Connect failed:', e);
      if (this._connCb) this._connCb(false);
    }
  }

  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
    }
    if (this._connCb) this._connCb(false);
  }

  // ---- Frame parser ----
  _rxByte(b) {
    switch (this._rxState) {
      case 0: if (b === SOF) this._rxState = 1; break;
      case 1: this._rxLen = b; this._rxPayload = []; this._rxState = 2; break;
      case 2: this._rxOp = b; this._rxState = 3; break;
      case 3:
        if (this._rxPayload.length < this._rxLen) this._rxPayload.push(b);
        if (this._rxPayload.length >= this._rxLen) this._rxState = 4;
        break;
      case 4: this._rxCrc = b; this._rxState = 5; break;
      case 5:
        if (b === EOF) this._dispatch(this._rxOp, this._rxPayload);
        this._rxState = 0;
        break;
    }
  }

  _dispatch(op, payload) {
    const p = new Uint8Array(payload);
    switch (op) {
      case OP.STATUS:
        if (this._statusCb) this._parseStatus(p);
        break;
      case OP.FRAME:
        if (this._frameCb) this._frameCb(this._parseFrame(p));
        break;
      case OP.RULE_LIST_RSP:
        if (this._rulesCb) this._rulesCb(this._parseRules(p));
        break;
      default:
        // ACK/NACK etc.
        break;
    }
  }

  _parseStatus(p) {
    const mode = p[0];
    const battPresent = p[1];
    const ruleCount = p[2];
    const tempRaw = p[3] | (p[4] << 8);
    const voltRaw = p[5] | (p[6] << 8);
    const tick = p[7];

    const modeNames = ['PassThrough', 'MITM', 'Jam', 'SpoofFull', 'Standby'];
    this._statusCb({
      mode: modeNames[mode] || 'Unknown',
      modeCode: mode,
      batteryPresent: !!battPresent,
      ruleCount,
      temperature: tempRaw,
      voltage: voltRaw,
      tick,
      timestamp: Date.now(),
    });
  }

  _parseFrame(p) {
    const ts = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    const port = p[4];
    const dir = p[5];
    const addr = p[6];
    const cmd = p[7];
    const wlen = p[8];
    const rlen = p[9];
    const flags = p[10];
    const wbuf = Array.from(p.slice(11, 11 + wlen));
    const rbuf = Array.from(p.slice(11 + wlen, 11 + wlen + rlen));
    return {
      ts,
      port: port === 0 ? 'HOST' : 'BATT',
      dir: ['Read', 'Write', 'Alert'][dir] || '?',
      addr: '0x' + addr.toString(16).padStart(2, '0'),
      cmd: '0x' + cmd.toString(16).padStart(2, '0'),
      cmdName: SBS_NAMES[cmd] || 'Unknown',
      wbuf, rbuf,
      flags,
      modified: !!(flags & 0x04),
      injected: !!(flags & 0x08),
      nack: !!(flags & 0x01),
    };
  }

  _parseRules(p) {
    const count = p[0];
    const rules = [];
    let off = 1;
    for (let i = 0; i < count && off + 3 < p.length; i++) {
      rules.push({
        cmd: '0x' + p[off].toString(16).padStart(2, '0'),
        mask: '0x' + p[off + 1].toString(16).padStart(2, '0'),
        action: ['None', 'Spoof', 'Block', 'Inject', 'Log', 'Glitch'][p[off + 2]] || '?',
        spoofLen: p[off + 3],
      });
      off += 4;
    }
    return rules;
  }

  // ---- TX ----
  _crc8(data) {
    let crc = 0x00;
    for (let i = 0; i < data.length; i++) {
      crc ^= data[i];
      for (let b = 0; b < 8; b++) {
        if (crc & 0x80) crc = ((crc << 1) ^ 0x07) & 0xFF;
        else            crc = (crc << 1) & 0xFF;
      }
    }
    return crc & 0xFF;
  }

  async sendCommand(op, payload) {
    if (!this.txChar) { console.log('TX char not ready'); return; }
    payload = payload || [];
    const len = payload.length;
    const frame = [SOF, len, op, ...payload];
    const crcInput = [len, op, ...payload];
    const crc = this._crc8(crcInput);
    frame.push(crc, EOF);

    const b64 = base64.encode(String.fromCharCode(...frame));
    try {
      await this.txChar.writeWithResponse(b64);
    } catch (e) {
      console.log('Write failed:', e);
    }
  }

  // Convenience
  addRule(cmd, mask, action, spoofBytes) {
    const payload = [cmd, mask, action, ...(spoofBytes || [])];
    this.sendCommand(OP.RULE_ADD, payload);
  }

  clearRules() { this.sendCommand(OP.RULE_CLR, []); }
  listRules() { this.sendCommand(OP.RULE_LIST, []); }
  setMode(mode) { this.sendCommand(OP.SET_MODE, [mode]); }
  fireGlitch(us) { this.sendCommand(OP.GLITCH, [us & 0xFF, (us >> 8) & 0xFF]); }
  authBypass() { this.sendCommand(OP.AUTH_BYPASS, []); }
  inject(addr, cmd, writeBytes) {
    const wlen = writeBytes ? writeBytes.length : 0;
    this.sendCommand(OP.INJECT, [addr, cmd, wlen, ...(writeBytes || [])]);
  }
}

export default new JouleBleService();