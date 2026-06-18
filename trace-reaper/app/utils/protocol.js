/**
 * TRACE-REAPER — BLE protocol + connection layer
 *
 * Implements the framed command protocol (see firmware/drivers/ble_bridge.c)
 * on top of react-native-ble-plx. Exposes a DeviceConnection class with
 * connect/disconnect, send-command, and a subscription API for the live
 * notifications (status, live trace, correlation best, result, tamper).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import { BleManager } from 'react-native-ble-plx';
import base64 from 'react-native-base64';
import { createContext } from 'react';

/* ---- Service / characteristic UUIDs (vendor-assigned) ---- */
export const TRACE_REAPER_SERVICE_UUID     = '0000a1b2-c3d4-4e5f-8a7b-9c0d1e2f3a4b';
export const TR_CMD_CHAR_UUID               = '0000a1b3-c3d4-4e5f-8a7b-9c0d1e2f3a4b';
export const TR_NOTIFY_CHAR_UUID            = '0000a1b4-c3d4-4e5f-8a7b-9c0d1e2f3a4b';

/* ---- Frame constants (mirror firmware) ---- */
const STX = 0xAA;
const ETX = 0x55;

export const CMD = {
  HELLO: 0x01, AUTH_PASSKEY: 0x02, GET_STATUS: 0x03, CONFIGURE: 0x04,
  ARM: 0x05, STOP: 0x06, GET_RESULT: 0x07, DUMP_TRACE: 0x08,
  LIST_SESSIONS: 0x09, OPEN_SESSION: 0x0A, SET_INPUT: 0x0B,
  SET_GAIN: 0x0C, SET_TRIG: 0x0D, WIPE: 0x0E,
};

export const NTF = {
  STATUS: 0x81, LIVE_TRACE: 0x82, CORR_BEST: 0x83, RESULT: 0x84,
  TAMPER: 0x85, FAULT: 0x86,
};

export const MODE = {
  IDLE: 0, CONFIGURED: 1, ARMED: 2, ACQUIRING: 3, PROCESSING: 4,
  DONE: 5, TAMPERED: 6, FAULT: 7,
};

export const CIPHER = { NONE:0, AES128:1, AES256:2, DES:3, PRESENT:4 };
export const LEAK = {
  NONE:0, HW_SBOX_OUT:1, HW_SBOX_IN:2, HD_SBOX_OUT:3,
  HW_MIXCOL:4, HW_LASTROUND:5,
};
export const TRIG = { EXTERNAL:0, ANALOG:1, TEMPLATE:2, NONE:3 };
export const INPUT = { SHUNT:0, EM:1 };
export const GAIN = { G0DB:0, G14DB:1, G28DB:2 };

/* ---- CRC8 (poly 0x07) ---- */
function crc8(bytes) {
  let c = 0;
  for (let i = 0; i < bytes.length; i++) {
    c ^= bytes[i];
    for (let b = 0; b < 8; b++) {
      c = (c & 0x80) ? ((c << 1) ^ 0x07) & 0xFF : (c << 1) & 0xFF;
    }
  }
  return c & 0xFF;
}

/* ---- Build a frame ---- */
function buildFrame(cmd, payload) {
  payload = payload || new Uint8Array(0);
  const len = 1 + payload.length;
  const f = new Uint8Array(6 + payload.length);
  f[0] = STX;
  f[1] = len & 0xFF;
  f[2] = (len >> 8) & 0xFF;
  f[3] = cmd;
  f.set(payload, 4);
  const crc = crc8(f.slice(3, 4 + payload.length));
  f[4 + payload.length] = crc;
  f[5 + payload.length] = ETX;
  return f;
}

/* ---- Parse one notification frame from a byte array ----
 * Returns { cmd, payload } or null if incomplete/invalid.
 */
function parseFrame(buf) {
  if (buf.length < 6) return null;
  if (buf[0] !== STX) return null;
  let len = buf[1] | (buf[2] << 8);
  if (buf.length < 6 + (len - 1)) return null;
  const cmd = buf[3];
  const payload = buf.slice(4, 4 + (len - 1));
  const crc = buf[4 + (len - 1)];
  const calc = crc8(buf.slice(3, 4 + (len - 1)));
  if (crc !== calc) return null;
  if (buf[5 + (len - 1)] !== ETX) return null;
  return { cmd, payload };
}

/* ---- Helpers to read typed values from a Uint8Array ---- */
function u16le(b, o) { return (b[o] | (b[o+1] << 8)) & 0xFFFF; }
function u32le(b, o) { return (b[o] | (b[o+1]<<8) | (b[o+2]<<16) | (b[o+3]<<24)) >>> 0; }
function i16le(b, o) { let v = u16le(b, o); return v > 0x7FFF ? v - 0x10000 : v; }

/* ---- Decode notification payloads into JS objects ---- */
export function decodeNotification(ntf) {
  switch (ntf.cmd) {
  case NTF.STATUS: {
    const p = ntf.payload;
    return {
      kind: 'status',
      uptime_s: u32le(p, 0),
      battery_pct: p[4],
      mode: p[5],
      traces: u16le(p, 6) | (p.length > 8 ? (p[8] << 16) : 0),
    };
  }
  case NTF.LIVE_TRACE: {
    const p = ntf.payload;
    const samples = new Int16Array(p.length / 2);
    for (let i = 0; i < samples.length; i++)
      samples[i] = i16le(p, i*2);
    return { kind: 'live_trace', samples };
  }
  case NTF.CORR_BEST: {
    const p = ntf.payload;
    const nbytes = p[32] || 16;
    const best = Array.from(p.slice(0, 16));
    const rho = [];
    for (let i = 0; i < 16; i++) rho[i] = p[16+i] / 100.0;
    return { kind: 'corr_best', best, rho, nbytes };
  }
  case NTF.RESULT: {
    const p = ntf.payload;
    const nbytes = (p.length - 2) / 2;
    const bestKey = Array.from(p.slice(0, nbytes));
    const rho = [];
    for (let i = 0; i < nbytes; i++) rho[i] = p[nbytes + i] / 100.0;
    return {
      kind: 'result', bestKey, rho, nbytes,
      recoveredBytes: p[nbytes*2], confidenceOk: p[nbytes*2+1],
    };
  }
  case NTF.TAMPER: return { kind: 'tamper' };
  case NTF.FAULT: {
    let s = '';
    for (let i = 0; i < ntf.payload.length; i++) s += String.fromCharCode(ntf.payload[i]);
    return { kind: 'fault', message: s };
  }
  default: return { kind: 'unknown', cmd: ntf.cmd };
  }
}

/* ---- DeviceConnection ---- */
export class DeviceConnection {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.connected = false;
    this.encrypted = false;
    this.authenticated = false;
    this.status = { mode: MODE.IDLE, uptime_s: 0, battery_pct: 0, traces: 0 };
    this.liveTrace = new Int16Array(0);
    this.corrBest = { best: [], rho: [], nbytes: 0 };
    this.result = null;
    this.tampered = false;
    this.fault = '';
    this.subscribers = new Set();
    this._rxBuf = [];
  }

  /* Subscribe to state updates; returns an unsubscribe fn. */
  subscribe(cb) {
    this.subscribers.add(cb);
    return () => this.subscribers.delete(cb);
  }

  _emit() {
    for (const cb of this.subscribers) cb(this._snapshot());
  }

  _snapshot() {
    return {
      connected: this.connected,
      encrypted: this.encrypted,
      authenticated: this.authenticated,
      status: { ...this.status },
      liveTrace: this.liveTrace,
      corrBest: this.corrBest,
      result: this.result,
      tampered: this.tampered,
      fault: this.fault,
    };
  }

  async startScan() {
    return new Promise((resolve, reject) => {
      this.manager.startDeviceScan([TRACE_REAPER_SERVICE_UUID], null, (err, dev) => {
        if (err) { reject(err); return; }
        this.manager.stopDeviceScan();
        this._connect(dev).then(resolve).catch(reject);
      });
    });
  }

  async _connect(dev) {
    try {
      this.device = await dev.connect();
      await this.device.discoverAllServicesAndCharacteristics();
      const chars = await this.device.characteristicsForService(TRACE_REAPER_SERVICE_UUID);
      this.cmdChar = chars.find(c => c.uuid.toLowerCase() === TR_CMD_CHAR_UUID.toLowerCase());
      this.ntfChar = chars.find(c => c.uuid.toLowerCase() === TR_NOTIFY_CHAR_UUID.toLowerCase());
      this.connected = true;
      /* Subscribe to notifications */
      this.ntfChar.monitor((err, char) => {
        if (err || !char || !char.value) return;
        const bytes = Uint8Array.from(base64.toByteArray(char.value));
        /* Buffer and split frames */
        for (let i = 0; i < bytes.length; i++) this._rxBuf.push(bytes[i]);
        this._tryConsumeFrames();
      }, 'TR_NOTIFY');
      this._emit();
      await this.sendCommand(CMD.HELLO);
    } catch (e) {
      this.connected = false;
      throw e;
    }
  }

  _tryConsumeFrames() {
    while (this._rxBuf.length >= 6) {
      const view = Uint8Array.from(this._rxBuf);
      const ntf = parseFrame(view);
      if (!ntf) {
        /* Drop leading byte if not STX */
        if (view[0] !== STX) this._rxBuf.shift();
        else break;
        continue;
      }
      const totalLen = 6 + (ntf.payload.length);
      this._rxBuf.splice(0, totalLen);
      const decoded = decodeNotification(ntf);
      this._applyNotification(decoded);
    }
    this._emit();
  }

  _applyNotification(d) {
    switch (d.kind) {
    case 'status':
      this.status = { mode: d.mode, uptime_s: d.uptime_s, battery_pct: d.battery_pct, traces: d.traces };
      break;
    case 'live_trace': this.liveTrace = d.samples; break;
    case 'corr_best':  this.corrBest = d; break;
    case 'result':     this.result = d; break;
    case 'tamper':     this.tampered = true; break;
    case 'fault':      this.fault = d.message; break;
    default: break;
    }
  }

  async sendCommand(cmd, payload) {
    if (!this.cmdChar) throw new Error('not connected');
    const frame = buildFrame(cmd, payload || new Uint8Array(0));
    const b64 = base64.fromByteArray(frame);
    await this.cmdChar.writeWithResponse(b64);
  }

  async authenticate(passkey) {
    /* Send the 6-digit passkey. In a real build, this is entered on the
     * device OLED (out-of-band) to defend against MITM. */
    const pk = new Uint8Array(6);
    for (let i = 0; i < 6 && i < passkey.length; i++) pk[i] = passkey.charCodeAt(i);
    await this.sendCommand(CMD.AUTH_PASSKEY, pk);
    this.authenticated = true;
  }

  async configure(cfg) {
    /* cfg is a SessionConfig object; we serialize to the firmware layout. */
    const p = new Uint8Array(88);
    p[0] = cfg.cipher;
    p[1] = cfg.model;
    /* target_traces u32 LE */
    p[2] = cfg.targetTraces & 0xFF;
    p[3] = (cfg.targetTraces >> 8) & 0xFF;
    p[4] = (cfg.targetTraces >> 16) & 0xFF;
    p[5] = (cfg.targetTraces >> 24) & 0xFF;
    /* window_samples u16 LE */
    p[6] = cfg.windowSamples & 0xFF;
    p[7] = (cfg.windowSamples >> 8) & 0xFF;
    /* trig_threshold i16 LE */
    p[8] = cfg.trigThreshold & 0xFF;
    p[9] = (cfg.trigThreshold >> 8) & 0xFF;
    p[10] = cfg.trigSrc;
    p[11] = cfg.input;
    p[12] = cfg.gain;
    /* known_pt[16] */
    for (let i = 0; i < 16 && i < cfg.knownPt.length; i++) p[13+i] = cfg.knownPt[i];
    p[29] = cfg.ptRandom ? 1 : 0;
    p[30] = cfg.decimate || 0;
    /* session_id[16] */
    for (let i = 0; i < 16; i++) p[31+i] = cfg.sessionId[i] || 0;
    /* label[32] */
    for (let i = 0; i < 32 && i < cfg.label.length; i++) p[47+i] = cfg.label.charCodeAt(i);
    await this.sendCommand(CMD.CONFIGURE, p);
  }

  async arm()      { await this.sendCommand(CMD.ARM); }
  async stop()     { await this.sendCommand(CMD.STOP); }
  async getResult(){ await this.sendCommand(CMD.GET_RESULT); }
  async dumpTrace(idx) {
    const p = new Uint8Array(4);
    p[0] = idx & 0xFF; p[1] = (idx >> 8) & 0xFF;
    p[2] = (idx >> 16) & 0xFF; p[3] = (idx >> 24) & 0xFF;
    await this.sendCommand(CMD.DUMP_TRACE, p);
  }
  async listSessions() { await this.sendCommand(CMD.LIST_SESSIONS); }
  async openSession(idx) {
    const p = new Uint8Array(4);
    p[0] = idx & 0xFF; p[1] = (idx >> 8) & 0xFF;
    p[2] = (idx >> 16) & 0xFF; p[3] = (idx >> 24) & 0xFF;
    await this.sendCommand(CMD.OPEN_SESSION, p);
  }
  async wipe() { await this.sendCommand(CMD.WIPE); }

  async disconnect() {
    try {
      if (this.device) await this.device.cancelConnection();
    } catch (e) { /* ignore */ }
    this.connected = false;
    this.encrypted = false;
    this.authenticated = false;
    this._emit();
  }
}

/* ---- React context ---- */
export const ConnectionContext = createContext(null);

/* ---- Helpers exported to screens ---- */
export function modeName(m) {
  return ['idle','configured','armed','acquiring','processing','done','tampered','fault'][m] || '?';
}

export function hex(b) {
  return Array.from(b).map(x => x.toString(16).padStart(2, '0')).join(' ');
}

export function rhoColor(r) {
  if (r >= 0.5) return '#2ecc71';
  if (r >= 0.25) return '#f1c40f';
  return '#e74c3c';
}