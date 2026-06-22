/**
 * protocol.js — WireReaper binary command protocol definitions
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Defines all command opcodes, response parsing, and helper
 * functions for constructing command payloads.
 */

import { Buffer } from 'buffer';

// ---- Command opcodes (must match firmware main.c) ----
export const CMD = {
  CAPTURE_START:    0x01,
  CAPTURE_STOP:     0x02,
  CAPTURE_STREAM:   0x03,
  I2C_SCAN:         0x10,
  I2C_READ:         0x11,
  I2C_WRITE:        0x12,
  I2C_EMULATE:      0x13,
  SPI_READ:         0x20,
  SPI_WRITE:        0x21,
  SPI_EMULATE:      0x22,
  UART_SNIFF:       0x30,
  UART_INJECT:      0x31,
  UART_SET_BAUD:    0x32,
  ONEWIRE_SCAN:     0x40,
  ONEWIRE_READ:     0x41,
  REPLAY_LOAD:      0x50,
  REPLAY_RUN:       0x51,
  FUZZ_START:       0x60,
  FUZZ_STOP:        0x61,
  STATUS:           0x70,
  VERSION:          0x71,
  RESET:            0xFF,
};

// ---- Bus type constants ----
export const BUS = {
  I2C:    0,
  SPI:    1,
  UART:   2,
  ONEWIRE: 3,
};

// ---- Channel counts (must match firmware) ----
export const CHANNELS = {
  I2C: 2,
  SPI: 2,
  UART: 4,
  ONEWIRE: 1,
};

// ---- Error codes ----
export const ERR = {
  OK: 0,
  BUSY: -1,
  PARAM: -2,
  TIMEOUT: -3,
  NACK: -4,
  OVERFLOW: -5,
  NOT_CONNECTED: -6,
  NO_TARGET: -7,
  HW_FAULT: -8,
};

/**
 * Build a CAPTURE_START payload.
 * @param {number} chanMask - bitmask of channels to capture
 * Author: jayis1
 */
export function buildCaptureStart(chanMask) {
  const buf = Buffer.alloc(4);
  buf.writeUInt32LE(chanMask, 0);
  return buf;
}

/**
 * Build an I2C_SCAN payload.
 * @param {number} channel - I2C channel (0-1)
 * Author: jayis1
 */
export function buildI2cScan(channel) {
  return Buffer.from([channel]);
}

/**
 * Build an I2C_READ payload.
 * @param {number} channel - I2C channel
 * @param {number} addr - 7-bit I2C address
 * @param {number} reg - register address
 * @param {number} length - bytes to read
 * Author: jayis1
 */
export function buildI2cRead(channel, addr, reg, length) {
  return Buffer.from([channel, addr & 0x7F, reg & 0xFF, length & 0xFF]);
}

/**
 * Build an I2C_WRITE payload.
 * @param {number} channel - I2C channel
 * @param {number} addr - 7-bit I2C address
 * @param {number} reg - register address
 * @param {number[]} data - bytes to write
 * Author: jayis1
 */
export function buildI2cWrite(channel, addr, reg, data) {
  return Buffer.from([channel, addr & 0x7F, reg & 0xFF, ...data]);
}

/**
 * Build an I2C_EMULATE payload.
 * @param {number} channel - I2C channel
 * @param {number} addr - 7-bit I2C address to emulate
 * Author: jayis1
 */
export function buildI2cEmulate(channel, addr) {
  return Buffer.from([channel, addr & 0x7F]);
}

/**
 * Build an SPI_READ payload.
 * @param {number} channel - SPI channel
 * @param {number[]} cmdBytes - command bytes to send
 * @param {number} readLen - bytes to read back
 * Author: jayis1
 */
export function buildSpiRead(channel, cmdBytes, readLen) {
  return Buffer.from([channel, cmdBytes.length, readLen, ...cmdBytes]);
}

/**
 * Build an SPI_WRITE payload.
 * @param {number} channel - SPI channel
 * @param {number[]} cmdBytes - command bytes
 * @param {number[]} data - data bytes
 * Author: jayis1
 */
export function buildSpiWrite(channel, cmdBytes, data) {
  return Buffer.from([channel, cmdBytes.length, ...cmdBytes, ...data]);
}

/**
 * Build a UART_SNIFF payload.
 * @param {number} channel - UART channel
 * Author: jayis1
 */
export function buildUartSniff(channel) {
  return Buffer.from([channel]);
}

/**
 * Build a UART_INJECT payload.
 * @param {number} channel - UART channel
 * @param {number[]} data - bytes to inject
 * Author: jayis1
 */
export function buildUartInject(channel, data) {
  return Buffer.from([channel, ...data]);
}

/**
 * Build a UART_SET_BAUD payload.
 * @param {number} channel - UART channel
 * @param {number} baud - baud rate
 * Author: jayis1
 */
export function buildUartSetBaud(channel, baud) {
  const buf = Buffer.alloc(5);
  buf.writeUInt8(channel, 0);
  buf.writeUInt32LE(baud, 1);
  return buf;
}

/**
 * Build a FUZZ_START payload.
 * @param {number} busType - BUS enum
 * @param {number} channel - channel to fuzz
 * Author: jayis1
 */
export function buildFuzzStart(busType, channel) {
  return Buffer.from([busType, channel]);
}

/**
 * Parse a STATUS response.
 * @param {Buffer} data - response data
 * @returns {object} parsed status
 * Author: jayis1
 */
export function parseStatus(data) {
  if (data.length < 20) return null;
  return {
    opcode: data[0],
    status: data[1],
    capturing: data[2] !== 0,
    streaming: data[3] !== 0,
    captureCount: data.readUInt32LE(4),
    fuzzActive: data[8] !== 0,
    fuzzIterations: data.readUInt32LE(9),
    uptime: data.readUInt32LE(13),
    battery: data[17],
    channelCount: data[18],
  };
}

/**
 * Parse a VERSION response.
 * @param {Buffer} data - response data
 * @returns {object} version info
 * Author: jayis1
 */
export function parseVersion(data) {
  if (data.length < 8) return null;
  const i2cCh = data[4] & 0xF;
  const spiCh = (data[4] >> 4) & 0xF;
  const uartCh = data[5] & 0xF;
  const owCh = (data[5] >> 4) & 0xF;
  const versionStr = data.slice(8).toString('utf8');
  return {
    major: data[1],
    minor: data[2],
    patch: data[3],
    i2cChannels: i2cCh,
    spiChannels: spiCh,
    uartChannels: uartCh,
    onewireChannels: owCh,
    versionString: versionStr,
  };
}

/**
 * Parse a stream of capture records.
 * @param {Buffer} data - raw record data
 * @returns {object[]} array of decoded records
 * Author: jayis1
 */
export function parseCaptureRecords(data) {
  const records = [];
  const RECORD_SIZE = 16;
  for (let i = 0; i + RECORD_SIZE <= data.length; i += RECORD_SIZE) {
    const rec = {
      bus: data[i],
      channel: data[i + 1],
      rw: data[i + 2],
      flags: data[i + 3],
      addr: data.readUInt16LE(i + 4),
      reg: data.readUInt16LE(i + 6),
      timestamp: data.readUInt32LE(i + 8),
      dataLen: data.readUInt16LE(i + 12),
      data: Array.from(data.slice(i + 14, i + 14 + Math.min(6, data.readUInt16LE(i + 12)))),
    };
    records.push(rec);
  }
  return records;
}

/**
 * Format a capture record as a human-readable string.
 * @param {object} rec - parsed record
 * @returns {string} formatted string
 * Author: jayis1
 */
export function formatRecord(rec) {
  const busNames = ['I2C', 'SPI', 'UART', '1WIRE'];
  const bus = busNames[rec.bus] || '???';
  const rw = rec.rw ? 'WR' : 'RD';
  const flag = rec.flags & 0x01 ? 'ACK' : rec.flags & 0x02 ? 'NACK' : '';
  const dataHex = rec.data.map(b => b.toString(16).padStart(2, '0')).join(' ');
  const ts = (rec.timestamp / 1000).toFixed(3);

  if (rec.bus === BUS.I2C) {
    return `[${ts}ms] ${bus} ch${rec.channel} ${rw} 0x${rec.addr.toString(16).padStart(2, '0')} reg=0x${rec.reg.toString(16).padStart(2, '0')} [${dataHex}] ${flag}`;
  } else if (rec.bus === BUS.SPI) {
    return `[${ts}ms] ${bus} ch${rec.channel} ${rw} cmd=0x${rec.reg.toString(16).padStart(2, '0')} [${dataHex}]`;
  } else if (rec.bus === BUS.UART) {
    const ascii = rec.data.map(b => (b >= 32 && b < 127) ? String.fromCharCode(b) : '.').join('');
    return `[${ts}ms] ${bus} ch${rec.channel} ${rw} [${dataHex}] "${ascii}"`;
  } else {
    return `[${ts}ms] ${bus} ch${rec.channel} [${dataHex}]`;
  }
}