/*
 * tpm-phantom — app/utils/protocol.js
 * Wire protocol implementation for the companion app.
 *
 * Mirrors the firmware wire_protocol.c. Encodes/decodes binary frames
 * exchanged with the tpm-phantom device over BLE (Nordic UART Service)
 * or USB CDC.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import base64 from 'base64-js';

// =============================================================
// Constants
// =============================================================
export const SOF0 = 0xa5;
export const SOF1 = 0x5a;

// Commands (app → device)
export const CMD_GET_STATUS   = 0x01;
export const CMD_START_CAP    = 0x02;
export const CMD_STOP_CAP     = 0x03;
export const CMD_GET_STATS    = 0x04;
export const CMD_SET_FILTER   = 0x05;
export const CMD_INJECT_LPC   = 0x06;
export const CMD_INJECT_SPI   = 0x07;
export const CMD_SET_LED      = 0x08;
export const CMD_RESET        = 0x09;
export const CMD_SD_STATUS    = 0x0a;
export const CMD_FLUSH_SD     = 0x0b;
export const CMD_GET_VERSION  = 0x0c;

// Responses (device → app)
export const RSP_STATUS       = 0x81;
export const RSP_TRANSACTION  = 0x82;
export const RSP_STATS        = 0x83;
export const RSP_SD_INFO      = 0x84;
export const RSP_VERSION      = 0x85;
export const RSP_ERROR        = 0x86;

// Capture modes
export const MODE_IDLE     = 0;
export const MODE_LPC      = 1;
export const MODE_SPI      = 2;
export const MODE_INJECT_LPC = 3;
export const MODE_INJECT_SPI = 4;

// =============================================================
// CRC-16/CCITT (poly 0x1021, init 0xFFFF) — matches firmware
// =============================================================
function crc16(data) {
  let crc = 0xffff;
  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8);
    for (let b = 0; b < 8; b++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ 0x1021) & 0xffff;
      } else {
        crc = (crc << 1) & 0xffff;
      }
    }
  }
  return crc;
}

// =============================================================
// Pack a frame: [SOF0][SOF1][LEN_lo][LEN_hi][CMD][payload][CRC_lo][CRC_hi]
// Returns a Uint8Array.
// =============================================================
export function packFrame(cmd, payload) {
  const pl = payload || new Uint8Array(0);
  const len = pl.length;
  const buf = new Uint8Array(7 + len);
  buf[0] = SOF0;
  buf[1] = SOF1;
  buf[2] = len & 0xff;
  buf[3] = (len >> 8) & 0xff;
  buf[4] = cmd;
  if (len > 0) {
    buf.set(pl, 5);
  }
  // CRC covers [LEN_lo, LEN_hi, CMD, payload]
  const crcData = new Uint8Array(3 + len);
  crcData[0] = buf[2];
  crcData[1] = buf[3];
  crcData[2] = buf[4];
  if (len > 0) crcData.set(pl, 3);
  const crc = crc16(crcData);
  buf[5 + len] = crc & 0xff;
  buf[6 + len] = (crc >> 8) & 0xff;
  return buf;
}

// =============================================================
// Frame parser — feed bytes incrementally, emits complete frames
// =============================================================
export class FrameParser {
  constructor(onFrame) {
    this.onFrame = onFrame;
    this.reset();
  }

  reset() {
    this.state = 0; // 0=sOF0, 1=sOF1, 2=lenLo, 3=lenHi, 4=cmd, 5=payload, 6=crcLo, 7=crcHi
    this.len = 0;
    this.cmd = 0;
    this.payload = [];
    this.idx = 0;
    this.crcLo = 0;
  }

  feed(data) {
    for (let i = 0; i < data.length; i++) {
      this._feedByte(data[i]);
    }
  }

  _feedByte(b) {
    switch (this.state) {
      case 0:
        if (b === SOF0) this.state = 1;
        break;
      case 1:
        if (b === SOF1) this.state = 2;
        else this.state = 0;
        break;
      case 2:
        this.len = b;
        this.state = 3;
        break;
      case 3:
        this.len |= (b << 8);
        this.state = 4;
        break;
      case 4:
        this.cmd = b;
        this.idx = 0;
        this.payload = new Uint8Array(this.len);
        this.state = (this.len > 0) ? 5 : 6;
        break;
      case 5:
        if (this.idx < this.len) {
          this.payload[this.idx++] = b;
        }
        if (this.idx >= this.len) {
          this.state = 6;
        }
        break;
      case 6:
        this.crcLo = b;
        this.state = 7;
        break;
      case 7: {
        const rcvd = this.crcLo | (b << 8);
        const crcData = new Uint8Array(3 + this.len);
        crcData[0] = this.len & 0xff;
        crcData[1] = (this.len >> 8) & 0xff;
        crcData[2] = this.cmd;
        if (this.len > 0) crcData.set(this.payload, 3);
        const calc = crc16(crcData);
        if (rcvd === calc) {
          this.onFrame(this.cmd, this.payload);
        }
        this.state = 0;
        break;
      }
      default:
        this.state = 0;
        break;
    }
  }
}

// =============================================================
// Decode helpers for specific response types
// =============================================================
export function decodeStatus(payload) {
  return {
    mode: payload[0],
    capturing: payload[1] !== 0,
    totalTx: u32le(payload, 2),
    tpmTx: u32le(payload, 6),
    sdReady: payload[10] !== 0,
    usbConnected: payload[11] !== 0,
  };
}

export function decodeStats(payload) {
  return {
    lpcTotal: u32le(payload, 0),
    lpcTpm: u32le(payload, 4),
    spiTotal: u32le(payload, 8),
    spiReads: u16le(payload, 12),
    spiWrites: u16le(payload, 14),
    spiBytes: u32le(payload, 16),
    sdBlocks: u32le(payload, 20),
    sdBytes: u32le(payload, 24),
    lpcSerirq: u32le(payload, 28),
  };
}

export function decodeTransaction(payload) {
  const bus = payload[0]; // 0=LPC, 1=SPI
  if (bus === 0) {
    return {
      bus: 'LPC',
      direction: payload[1] ? 'READ' : 'WRITE',
      cycleType: payload[2],
      isTpm: payload[3] !== 0,
      address: u32le(payload, 4),
      data: payload[8],
      timestamp: u32le(payload, 9),
    };
  } else {
    return {
      bus: 'SPI',
      command: payload[1],
      isTpm: payload[2] !== 0,
      dataLen: payload[3],
      address: (payload[4] | (payload[5] << 8) | (payload[6] << 16)),
      timestamp: u32le(payload, 7),
      data: Array.from(payload.slice(11, 11 + payload[3])),
    };
  }
}

export function decodeVersion(payload) {
  const len = payload[0];
  const bytes = payload.slice(1, 1 + len);
  return new TextDecoder().decode(bytes);
}

export function decodeSdInfo(payload) {
  return {
    ready: payload[0] !== 0,
    blocks: u32le(payload, 1),
    errors: u32le(payload, 5),
  };
}

// =============================================================
// Command builders — return Uint8Array payloads for packFrame()
// =============================================================
export function cmdStartCapture(mode, flags) {
  return new Uint8Array([mode, flags || 0]);
}

export function cmdSetFilter(addrMask, addrVal) {
  const p = new Uint8Array(8);
  p[0] = addrMask & 0xff;
  p[1] = (addrMask >> 8) & 0xff;
  p[2] = (addrMask >> 16) & 0xff;
  p[3] = (addrMask >> 24) & 0xff;
  p[4] = addrVal & 0xff;
  p[5] = (addrVal >> 8) & 0xff;
  p[6] = (addrVal >> 16) & 0xff;
  p[7] = (addrVal >> 24) & 0xff;
  return p;
}

export function cmdInjectLpc(addr, data) {
  return new Uint8Array([addr & 0xff, (addr >> 8) & 0xff, data & 0xff]);
}

export function cmdInjectSpi(dataArray) {
  const p = new Uint8Array(1 + dataArray.length);
  p[0] = dataArray.length;
  p.set(dataArray, 1);
  return p;
}

export function cmdSetLed(led, state) {
  return new Uint8Array([led, state ? 1 : 0]);
}

// =============================================================
// Encoding helpers
// =============================================================
function u32le(buf, off) {
  return buf[off] | (buf[off+1] << 8) | (buf[off+2] << 16) | (buf[off+3] << 24);
}
function u16le(buf, off) {
  return buf[off] | (buf[off+1] << 8);
}

export function bufToBase64(buf) {
  return base64.fromByteArray(buf);
}

export function base64ToBuf(str) {
  return base64.toByteArray(str);
}

// =============================================================
// TPM register name decoder (for display)
// =============================================================
const TPM_REG_NAMES = {
  0x00: 'TPM_ACCESS',
  0x04: 'TPM_STS',
  0x08: 'TPM_HASH_START',
  0x0c: 'TPM_DATA_FIFO',
  0x18: 'TPM_INTERFACE_ID',
  0x24: 'TPM_DID_VID',
  0x2c: 'TPM_RID',
  0x38: 'TPM_XFER_SIZE',
  0x3c: 'TPM_INTEGRITY_PORT',
};

export function tpmRegisterName(addr) {
  const offset = addr & 0xfff;
  if (TPM_REG_NAMES[offset]) return TPM_REG_NAMES[offset];
  if (offset >= 0x80 && offset <= 0x84) return `TPM_LOCALITY_${(offset >> 4) & 7}`;
  return `0x${addr.toString(16).toUpperCase()}`;
}

// TPM locality (0-4) decode
export function tpmLocality(addr) {
  return (addr >> 12) & 0x7;
}