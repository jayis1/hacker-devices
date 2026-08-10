/**
 * protocol.js — Wire protocol for Ember-Tap companion app
 *
 * Author: jayis1
 * License: MIT
 *
 * Defines the command/response packet format used over USB-CDC or
 * Bluetooth SPP to communicate with the Ember-Tap device.
 */

'use strict';

/* Packet format:
 *   [0x55] [CMD] [LEN_LO] [LEN_HI] [PAYLOAD...] [CRC8] [0xAA]
 *
 * Commands are ASCII-mapped for human-readable CDC use, but this
 * binary framing is used by the app for reliable machine parsing.
 */

export const SYNC_IN  = 0x55;
export const SYNC_OUT = 0xAA;

/* Command opcodes */
export const CMD = {
  STATUS:       0x01,
  SNIFF_ON:     0x02,
  SNIFF_OFF:    0x03,
  SPOOF_SRC:    0x04,
  SINK_REQ:     0x05,
  FUZZ_START:   0x06,
  FUZZ_STOP:    0x07,
  FUZZ_PROFILE: 0x08,
  HARD_RESET:   0x09,
  ROLE_SWAP:    0x0A,
  DEAD_BATTERY: 0x0B,
  VBUS_GLITCH:  0x0C,
  LOG_DUMP:     0x0D,
  SET_OVP:      0x0E,
  SET_ILIM:     0x0F,
};

/* Response opcodes (device → app) */
export const RSP = {
  OK:           0x80,
  ERROR:        0x81,
  STATUS_DATA:  0x82,
  PD_FRAME:     0x83,
  FUZZ_PROGRESS:0x84,
  LOG_DATA:     0x85,
};

/* Fuzz profile codes */
export const FUZZ_PROFILES = {
  HEADER:  0,
  PDO:     1,
  TIMING:  2,
  CHUNK:   3,
};

/* Mode names */
export const MODE_NAMES = [
  'PASSIVE',
  'SPOOF-SRC',
  'SINK-MASQ',
  'FUZZ',
  'GLITCH',
];

/* ---- CRC-8 (poly 0x07, init 0x00) ---- */
function crc8(data) {
  let crc = 0;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 0x80) crc = ((crc << 1) ^ 0x07) & 0xFF;
      else            crc = (crc << 1) & 0xFF;
    }
  }
  return crc;
}

/* ---- Build a command packet ---- */
export function buildPacket(cmd, payload = []) {
  const len = payload.length;
  const buf = [SYNC_IN, cmd, len & 0xFF, (len >> 8) & 0xFF, ...payload];
  const crc = crc8(buf.slice(1));  /* CRC over cmd..payload */
  buf.push(crc);
  buf.push(SYNC_OUT);
  return Buffer.from(buf);
}

/* ---- Parse an incoming byte stream ----
 * Returns an array of decoded packets. Maintains internal state
 * across calls via the `state` object.
 */
export function parseStream(data, state = { buf: [], sync: false }) {
  const packets = [];
  for (const byte of data) {
    if (!state.sync) {
      if (byte === SYNC_IN) {
        state.sync = true;
        state.buf = [byte];
      }
      continue;
    }
    state.buf.push(byte);
    if (byte === SYNC_OUT && state.buf.length >= 6) {
      const pkt = state.buf;
      const cmd = pkt[1];
      const len = pkt[2] | (pkt[3] << 8);
      if (pkt.length === len + 6) {
        const crc = pkt[pkt.length - 2];
        const calc = crc8(pkt.slice(1, pkt.length - 2));
        if (crc === calc) {
          packets.push({
            cmd,
            payload: pkt.slice(4, 4 + len),
          });
        }
      }
      state.buf = [];
      state.sync = false;
    }
  }
  return packets;
}

/* ---- Decode a STATUS_DATA payload ---- */
export function decodeStatus(payload) {
  if (payload.length < 14) return null;
  return {
    mode:       payload[0],
    vbus_mv:    payload[1] | (payload[2] << 8),
    vbus_ma:    payload[3] | (payload[4] << 8),
    temp_c:     payload[5] > 127 ? payload[5] - 256 : payload[5],
    fuzz_sent:  payload[6] | (payload[7] << 8) | (payload[8] << 16) | (payload[9] << 24),
    fuzz_crash: payload[10] | (payload[11] << 8) | (payload[12] << 16) | (payload[13] << 24),
  };
}

/* ---- Decode a PD_FRAME payload ---- */
export function decodePDFrame(payload) {
  if (payload.length < 8) return null;
  const ts = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
  const sof = payload[4];
  const hdrRaw = payload[5] | (payload[6] << 8);
  const numobj = (hdrRaw >> 14) & 0x3;
  return {
    ts_ms: ts,
    sof: sof,
    msg_type: hdrRaw & 0x1F,
    spec_rev: (hdrRaw >> 6) & 0x3,
    power_role: (hdrRaw >> 8) & 0x1,
    data_role: (hdrRaw >> 5) & 0x1,
    msg_id: (hdrRaw >> 9) & 0x1F,
    numobj: numobj,
    raw: hdrRaw,
  };
}

/* ---- Encode a SPOOF_SRC command ---- */
export function encodeSpoofSrc(mv, ma) {
  /* Fixed PDO: bit31=Fixed, bits20-31=V/50mV, bits10-19=I/10mA */
  const pdo = (1 << 31) | ((Math.floor(ma / 10) & 0x3FF) << 10) | ((Math.floor(mv / 50) & 0xFFF) << 20);
  return [
    pdo & 0xFF,
    (pdo >> 8) & 0xFF,
    (pdo >> 16) & 0xFF,
    (pdo >> 24) & 0xFF,
  ];
}

/* ---- Encode a VBUS_GLITCH command ---- */
export function encodeGlitch(us, type) {
  return [
    us & 0xFF,
    (us >> 8) & 0xFF,
    (us >> 16) & 0xFF,
    type & 0xFF,
  ];
}

/* ---- Encode a FUZZ_START command ---- */
export function encodeFuzzStart(count, seed) {
  return [
    count & 0xFF,
    (count >> 8) & 0xFF,
    (count >> 16) & 0xFF,
    (count >> 24) & 0xFF,
    seed & 0xFF,
    (seed >> 8) & 0xFF,
    (seed >> 16) & 0xFF,
    (seed >> 24) & 0xFF,
  ];
}

/* ---- Connection manager ----
 * Abstracts the serial/Bluetooth transport. The app calls connect(),
 * then send(cmd, payload) and registers a callback for received packets.
 */
export class EmberTapConnection {
  constructor() {
    this.state = { buf: [], sync: false };
    this.onPacket = null;
    this.transport = null;
  }

  async connect(transport) {
    this.transport = transport;
    await transport.open();
    transport.onData((data) => {
      const packets = parseStream(Array.from(data), this.state);
      for (const pkt of packets) {
        if (this.onPacket) this.onPacket(pkt);
      }
    });
  }

  async send(cmd, payload = []) {
    const pkt = buildPacket(cmd, payload);
    await this.transport.write(pkt);
  }

  async disconnect() {
    if (this.transport) {
      await this.transport.close();
      this.transport = null;
    }
  }
}

/* end of file — author: jayis1 */