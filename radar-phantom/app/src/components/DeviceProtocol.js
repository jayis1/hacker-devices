/**
 * DeviceProtocol.js — Binary frame codec for RadarPhantom BLE/USB
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Packs and unpacks the STX/LEN/OP/PAYLOAD/CRC8/ETX frame protocol used by
 * the RadarPhantom firmware (see drivers/ble_uart.c). Opcodes match the
 * table documented in the README.
 */

// Opcodes
export const OP = {
  PING:        0x01,
  SET_RANGE:   0x02,
  SET_VEL:     0x03,
  SET_RCS:     0x04,
  SET_TAPS:    0x05,
  LOAD_SCN:    0x06,
  SNIFF:       0x07,
  ARM:         0x08,
  DISARM:      0x09,
  LOG_DUMP:    0x0A,
  SET_POWER:   0x0B,
};

// Response opcodes (request | 0x80)
export const RSP = {
  PING:        0x81,
  SET_RANGE:   0x82,
  SET_VEL:     0x83,
  SET_RCS:     0x84,
  SET_TAPS:    0x85,
  LOAD_SCN:    0x86,
  SNIFF:       0x87,
  ARM:         0x88,
  DISARM:      0x89,
  LOG_DUMP:    0x8A,
  SET_POWER:   0x8B,
  UNKNOWN:     0xFF,
};

const STX = 0xA5;
const ETX = 0x5A;

// CRC-8 (polynomial 0x07, init 0x00)
function crc8(data) {
  let crc = 0;
  for (const b of data) {
    crc ^= b;
    for (let i = 0; i < 8; i++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) & 0xFF : (crc << 1) & 0xFF;
    }
  }
  return crc;
}

// Encode a frame: returns a Uint8Array
export function encodeFrame(opcode, payload = []) {
  const plen = payload.length;
  if (plen > 60) throw new Error('payload too long');
  const len = plen + 1;   // includes opcode
  const buf = new Uint8Array(2 + len + 1 + 1);  // STX, LEN, OP, PAY, CRC, ETX
  buf[0] = STX;
  buf[1] = len;
  buf[2] = opcode;
  for (let i = 0; i < plen; i++) buf[3 + i] = payload[i];
  buf[3 + plen] = crc8(new Uint8Array([opcode, ...payload]));
  buf[4 + plen] = ETX;
  return buf;
}

// Decode a stream of bytes; returns parsed frames via callback
export class FrameDecoder {
  constructor() {
    this.state = 0;   // 0=idle 1=gotSTX 2=gotLEN 3=gotPayload 4=gotCRC
    this.plen = 0;
    this.idx = 0;
    this.buf = new Uint8Array(64);
  }

  /** Feed bytes; calls onFrame(opcode, payload) for each complete frame */
  feed(bytes, onFrame) {
    for (const byte of bytes) {
      switch (this.state) {
        case 0:
          if (byte === STX) this.state = 1;
          break;
        case 1:
          if (byte === 0 || byte > 63) { this.state = 0; break; }
          this.plen = byte;
          this.idx = 0;
          this.state = 2;
          break;
        case 2:
          this.buf[this.idx++] = byte;
          if (this.idx >= this.plen) this.state = 3;
          break;
        case 3:
          this.buf[this.idx] = byte;   // CRC
          this.state = 4;
          break;
        case 4:
          if (byte === ETX) {
            const expected = crc8(this.buf.slice(0, this.plen));
            if (expected === this.buf[this.plen]) {
              onFrame(this.buf[0], this.buf.slice(1, this.plen));
            }
          }
          this.state = 0;
          break;
      }
    }
  }
}

// Payload helpers
export function u32(n) {
  return [n & 0xFF, (n >> 8) & 0xFF, (n >> 16) & 0xFF, (n >>> 24) & 0xFF];
}
export function s32(n) {
  return u32(n >>> 0);
}
export function s16(n) {
  return [n & 0xFF, (n >> 8) & 0xFF];
}

export function parseU32(payload, off = 0) {
  return (payload[off] |
          (payload[off + 1] << 8) |
          (payload[off + 2] << 16) |
          (payload[off + 3] << 24)) >>> 0;
}

export function parseS32(payload, off = 0) {
  return parseU32(payload, off) | 0;
}

export function parseU64(payload, off = 0) {
  // 5-byte little-endian used by firmware for Hz
  let v = 0;
  for (let i = 0; i < 5; i++) v += BigInt(payload[off + i]) << BigInt(i * 8);
  return v;
}