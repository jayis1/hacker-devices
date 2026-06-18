/*
 * utils/protocol.js — BLE C2 protocol for GNSS-Phantom companion app
 *
 * Implements the framed, CRC-16/CCITT protected command/response protocol
 * used to communicate with the GNSS-Phantom device over Nordic UART Service
 * (NUS) BLE characteristics.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

// NUS BLE service/characteristic UUIDs (Nordic UART Service)
export const NUS_SERVICE_UUID     = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
export const NUS_TX_CHAR_UUID      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write to device
export const NUS_RX_CHAR_UUID      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify from device

// Protocol framing
const SOF = 0xA5;
const EOF = 0x5A;
const DLE = 0x7D;

// C2 command opcodes (must match firmware board.h)
export const CMD = {
  PING:             0x01,
  GET_STATUS:        0x02,
  SET_FREQ:          0x10,
  SET_POWER:         0x11,
  SET_SV:            0x12,
  LOAD_EPHEMERIS:    0x20,
  LOAD_SCENARIO:    0x21,
  START_SPOOF:       0x30,
  STOP_SPOOF:        0x31,
  START_RX:          0x32,
  STOP_RX:           0x33,
  SET_TRAJECTORY:    0x40,
  SET_TIME_OFFSET:  0x41,
  ARM:               0x50,
  DISARM:            0x51,
  FACTORY_RESET:    0xFF,
};

// Response codes
export const RSP = {
  OK:         0x00,
  ERROR:      0x01,
  BUSY:       0x02,
  UNARMED:   0x03,
  BAD_PARAM: 0x04,
};

// Constellation types
export const CONSTELLATION = {
  GPS:      0,
  GLONASS:  1,
  GALILEO:  2,
  BEIDOU:   3,
};

// Engine states
export const ENGINE_STATE = {
  0: 'IDLE',
  1: 'ARMED',
  2: 'TRANSMITTING',
  3: 'RX_CALIBRATE',
};

// RF frequencies in MHz
export const RF_FREQS = {
  GPS_L1:     1575.420,
  GALILEO_E1: 1575.420,
  GLONASS_L1: 1602.000,
  BEIDOU_B1:  1561.098,
};

// CRC-16/CCITT-FALSE
function crc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i] << 8;
    for (let b = 0; b < 8; b++) {
      if (crc & 0x8000)
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      else
        crc = (crc << 1) & 0xFFFF;
    }
  }
  return crc;
}

// Escape special bytes for SLIP-like framing
function escapeByte(b) {
  if (b === DLE) return [DLE, 0x5D];
  if (b === SOF) return [DLE, 0x5A];
  if (b === EOF) return [DLE, 0x54];
  return [b];
}

// Encode a command + payload into a framed buffer
export function encodeFrame(cmd, payload = []) {
  const len = payload.length;
  const crcData = new Uint8Array(3 + len);
  crcData[0] = (len >> 8) & 0xFF;
  crcData[1] = len & 0xFF;
  crcData[2] = cmd;
  for (let i = 0; i < len; i++) crcData[3 + i] = payload[i];
  const crc = crc16(crcData);

  const raw = [SOF, (len >> 8) & 0xFF, len & 0xFF, cmd, ...payload,
              (crc >> 8) & 0xFF, crc & 0xFF, EOF];

  // Escape
  const escaped = [];
  for (const b of raw) {
    const esc = escapeByte(b);
    escaped.push(...esc);
  }
  return Buffer.from(escaped);
}

// Decode a received frame (called with notification data chunks)
export class FrameDecoder {
  constructor() {
    this.reset();
  }

  reset() {
    this.state = 'SOF';
    this.buf = [];
    this.payloadLen = 0;
    this.cmd = 0;
    this.escaped = false;
  }

  // Feed a byte array; returns {cmd, payload} when a complete frame is decoded
  feed(data) {
    const frames = [];
    for (const byte of data) {
      let b = byte;
      // Handle DLE escaping
      if (b === DLE && !this.escaped) {
        this.escaped = true;
        continue;
      }
      if (this.escaped) {
        if (b === 0x5D) b = DLE;
        else if (b === 0x5A) b = SOF;
        else if (b === 0x54) b = EOF;
        this.escaped = false;
      }

      switch (this.state) {
        case 'SOF':
          if (b === SOF) this.state = 'LEN_HI';
          break;
        case 'LEN_HI':
          this.payloadLen = b << 8;
          this.state = 'LEN_LO';
          break;
        case 'LEN_LO':
          this.payloadLen |= b;
          this.state = 'CMD';
          break;
        case 'CMD':
          this.cmd = b;
          this.buf = [];
          if (this.payloadLen > 0) this.state = 'PAYLOAD';
          else this.state = 'CRC_HI';
          break;
        case 'PAYLOAD':
          this.buf.push(b);
          if (this.buf.length >= this.payloadLen) this.state = 'CRC_HI';
          break;
        case 'CRC_HI':
          this.state = 'CRC_LO';
          break;
        case 'CRC_LO':
          this.state = 'EOF_WAIT';
          break;
        case 'EOF_WAIT':
          if (b === EOF) {
            frames.push({ cmd: this.cmd, payload: new Uint8Array(this.buf) });
          }
          this.reset();
          break;
      }
    }
    return frames;
  }
}

// Build specific commands
export function cmdPing() {
  return encodeFrame(CMD.PING);
}

export function cmdGetStatus() {
  return encodeFrame(CMD.GET_STATUS);
}

export function cmdSetFreq(freqHz) {
  const p = new Uint8Array(4);
  p[0] = (freqHz >> 24) & 0xFF;
  p[1] = (freqHz >> 16) & 0xFF;
  p[2] = (freqHz >> 8) & 0xFF;
  p[3] = freqHz & 0xFF;
  return encodeFrame(CMD.SET_FREQ, p);
}

export function cmdSetPower(dbm) {
  const p = new Uint8Array(1);
  p[0] = dbm & 0xFF;
  return encodeFrame(CMD.SET_POWER, p);
}

export function cmdSetSV(constellation, prn, powerDb) {
  const p = new Uint8Array(3);
  p[0] = constellation & 0xFF;
  p[1] = prn & 0xFF;
  p[2] = powerDb & 0xFF;
  return encodeFrame(CMD.SET_SV, p);
}

export function cmdSetTrajectory(lat, lon, alt) {
  const p = new Uint8Array(24);
  // double lat (8 bytes, big-endian)
  const latBuf = new Float64Array([lat]);
  const latBytes = new Uint8Array(latBuf.buffer);
  for (let i = 0; i < 8; i++) p[i] = latBytes[7 - i]; // reverse for big-endian
  const lonBuf = new Float64Array([lon]);
  const lonBytes = new Uint8Array(lonBuf.buffer);
  for (let i = 0; i < 8; i++) p[8 + i] = lonBytes[7 - i];
  // alt as int32
  const altInt = Math.round(alt);
  p[16] = (altInt >> 24) & 0xFF;
  p[17] = (altInt >> 16) & 0xFF;
  p[18] = (altInt >> 8) & 0xFF;
  p[19] = altInt & 0xFF;
  return encodeFrame(CMD.SET_TRAJECTORY, p);
}

export function cmdSetTime(week, towMs) {
  const p = new Uint8Array(8);
  p[0] = (week >> 24) & 0xFF;
  p[1] = (week >> 16) & 0xFF;
  p[2] = (week >> 8) & 0xFF;
  p[3] = week & 0xFF;
  p[4] = (towMs >> 24) & 0xFF;
  p[5] = (towMs >> 16) & 0xFF;
  p[6] = (towMs >> 8) & 0xFF;
  p[7] = towMs & 0xFF;
  return encodeFrame(CMD.SET_TIME_OFFSET, p);
}

export function cmdArm() {
  return encodeFrame(CMD.ARM);
}

export function cmdDisarm() {
  return encodeFrame(CMD.DISARM);
}

export function cmdStartSpoof() {
  return encodeFrame(CMD.START_SPOOF);
}

export function cmdStopSpoof() {
  return encodeFrame(CMD.STOP_SPOOF);
}

export function cmdLoadEphemeris(prn, ephemerisBytes) {
  const p = new Uint8Array(1 + ephemerisBytes.length);
  p[0] = prn & 0xFF;
  for (let i = 0; i < ephemerisBytes.length; i++) p[1 + i] = ephemerisBytes[i];
  return encodeFrame(CMD.LOAD_EPHEMERIS, p);
}

// Parse status response from device
export function parseStatus(payload) {
  if (payload.length < 8) return null;
  return {
    engineState: payload[0],
    engineStateName: ENGINE_STATE[payload[0]] || 'UNKNOWN',
    svCount: payload[1],
    batteryMv: (payload[2] << 8) | payload[3],
    tempC: payload[4] > 127 ? payload[4] - 256 : payload[4],
    armed: payload[5] === 1,
    safetyEngaged: payload[6] === 1,
  };
}