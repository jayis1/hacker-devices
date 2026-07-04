/**
 * AperturePhantom — binary command protocol codec
 *
 * Encodes/decodes the framed protocol used over BLE GATT and USB CDC:
 *   [STX=0xAA][LEN(2 BE)][OP][PAYLOAD..][CRC8][ETX=0x55]
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import base64 from 'react-native-base64';

export const CMD = {
  NOP: 0x00, PING: 0x01, GET_STATUS: 0x02, GET_LINK_INFO: 0x03,
  SET_MODE: 0x04, ARM: 0x05, CAP_START: 0x06, CAP_STOP: 0x07,
  CAP_SNAPSHOT: 0x08, CAP_STREAM: 0x09, INJECT_LOAD: 0x0A,
  INJECT_NOW: 0x0B, INJECT_LOOP: 0x0C, INJECT_STOP: 0x0D,
  REPLAY_LIST: 0x0E, REPLAY_START: 0x0F, REPLAY_STOP: 0x10,
  SCRIPT_LOAD: 0x11, SCRIPT_RUN: 0x12, SCRIPT_STOP: 0x13,
  SCRIPT_STATUS: 0x14, SENSOR_SCAN: 0x15, SENSOR_READ: 0x16,
  SENSOR_WRITE: 0x17, SENSOR_AUTOCONFIG: 0x18, FUZZ_START: 0x19,
  FUZZ_STOP: 0x1A, TRIGGER_SET: 0x1B, TRIGGER_ENABLE: 0x1C,
  TRIGGER_DISABLE: 0x1D, TOUCH_EVENT: 0x1E, AUTH_CHALLENGE: 0x1F,
  LOG: 0x20, FRAME: 0x21, ERROR: 0x7F,
};

export const MODE = {
  PASSTHROUGH: 0, CAPTURE: 1, INJECT: 2, REPLAY: 3, FUZZ: 4,
};

const STX = 0xAA, ETX = 0x55;

/* CRC8 (poly 0x07, init 0x00) — matches firmware registers.h crc8() */
export function crc8(bytes) {
  let c = 0x00;
  for (const b of bytes) {
    c ^= b;
    for (let i = 0; i < 8; i++) {
      c = (c & 0x80) ? ((c << 1) ^ 0x07) & 0xFF : (c << 1) & 0xFF;
    }
  }
  return c & 0xFF;
}

/* Encode a command into a Uint8Array frame. */
export function encodeFrame(op, payload) {
  const pl = payload ? new Uint8Array(payload) : new Uint8Array(0);
  const len = pl.length;
  const buf = new Uint8Array(5 + len + 2); /* STX, LEN(2), OP, payload, CRC, ETX */
  buf[0] = STX;
  buf[1] = (len >> 8) & 0xFF;
  buf[2] = len & 0xFF;
  buf[3] = op & 0xFF;
  buf.set(pl, 4);
  const crcInput = new Uint8Array(3 + len);
  crcInput[0] = buf[1]; crcInput[1] = buf[2]; crcInput[2] = buf[3];
  if (len) crcInput.set(pl, 3);
  buf[4 + len] = crc8(crcInput);
  buf[5 + len] = ETX;
  return buf;
}

/* Frame parser: feed bytes via push(), get complete frames via next(). */
export class FrameParser {
  constructor() {
    this._buf = new Uint8Array(2048);
    this._w = 0;
    this._state = 0; this._expect = 0; this._op = 0; this._len = 0;
  }

  push(bytes) {
    for (const b of bytes) {
      if (this._w >= this._buf.length) this._reset();
      this._buf[this._w++] = b & 0xFF;
    }
  }

  _reset() { this._w = 0; this._state = 0; }

  /* Returns { op, payload: Uint8Array } or null if no full frame yet. */
  next() {
    while (true) {
      if (this._w === 0) return null;
      switch (this._state) {
      case 0:
        if (this._buf[0] !== STX) { this._shift(1); continue; }
        if (this._w < 5) return null;
        this._state = 1; continue;
      case 1:
        this._expect = (this._buf[1] << 8) | this._buf[2];
        this._op = this._buf[3];
        this._len = 0;
        this._state = 2; continue;
      case 2:
        if (this._w < 5 + this._expect + 2) return null;
        /* got full frame: verify CRC + ETX */
        const crcIdx = 4 + this._expect;
        const crcInput = new Uint8Array(3 + this._expect);
        crcInput[0] = this._buf[1]; crcInput[1] = this._buf[2];
        crcInput[2] = this._buf[3];
        for (let i = 0; i < this._expect; i++)
          crcInput[3 + i] = this._buf[4 + i];
        const calc = crc8(crcInput);
        if (calc !== this._buf[crcIdx] || this._buf[crcIdx + 1] !== ETX) {
          this._shift(1); this._state = 0; continue;
        }
        const payload = new Uint8Array(this._expect);
        for (let i = 0; i < this._expect; i++)
          payload[i] = this._buf[4 + i];
        const op = this._op;
        this._shift(crcIdx + 2);
        this._state = 0;
        return { op, payload };
      default: this._state = 0; this._w = 0; continue;
      }
    }
  }

  _shift(n) {
    if (n >= this._w) { this._w = 0; return; }
    for (let i = 0; i < this._w - n; i++) this._buf[i] = this._buf[i + n];
    this._w -= n;
  }
}

/* Convenience helpers for building common payloads. */
export function buildSetMode(mode) {
  return encodeFrame(CMD.SET_MODE, [mode & 0xFF]);
}
export function buildArm(on) {
  return encodeFrame(CMD.ARM, [on ? 1 : 0]);
}
export function buildSensorRead(addr, reg, len) {
  return encodeFrame(CMD.SENSOR_READ, [addr & 0xFF, (reg >> 8) & 0xFF,
    reg & 0xFF, len & 0xFF]);
}
export function buildSensorWrite(addr, reg, val) {
  return encodeFrame(CMD.SENSOR_WRITE, [addr & 0xFF, (reg >> 8) & 0xFF,
    reg & 0xFF, (val >> 8) & 0xFF, val & 0xFF]);
}
export function buildInjectLoad(slot, frameBytes) {
  const p = new Uint8Array(1 + frameBytes.length);
  p[0] = slot & 0xFF;
  p.set(frameBytes, 1);
  return encodeFrame(CMD.INJECT_LOAD, p);
}
export function buildFuzzStart(strategy, count, delayMs) {
  return encodeFrame(CMD.FUZZ_START, [
    strategy & 0xFF, (count >> 8) & 0xFF, count & 0xFF,
    (delayMs >> 8) & 0xFF, delayMs & 0xFF,
  ]);
}
export function buildScriptLoad(bytecode) {
  return encodeFrame(CMD.SCRIPT_LOAD, bytecode);
}

/* Convert a Uint8Array to base64 for BLE write and vice-versa. */
export function toBase64(u8) { return base64.fromBytes(Array.from(u8)); }
export function fromBase64(b64) {
  const arr = base64.toBytes(b64);
  return new Uint8Array(arr);
}