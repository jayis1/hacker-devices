// src/ble/protocol.ts — BLE protocol encode/decode for Chronos-Phantom
//
// Implements the length-prefixed binary protocol matching firmware ble_link.c.
// Frame format: [SOF=0xAA][LEN_HI][LEN_LO][CMD/EVT][payload...][CRC8]
// LEN = payload + opcode + CRC8
//
// Author: jayis1
// License: GPL-2.0

import { CMD, EVT } from '../types';

const SOF = 0xAA;

// CRC-8/MAXIM (polynomial 0x31, init 0x00)
export function crc8(data: Uint8Array, len: number): number {
  let crc = 0;
  for (let i = 0; i < len; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 0x80) crc = ((crc << 1) ^ 0x31) & 0xFF;
      else crc = (crc << 1) & 0xFF;
    }
  }
  return crc;
}

export interface BleFrame {
  opcode: number;
  payload: Uint8Array;
}

export function encodeFrame(opcode: number, payload: Uint8Array): Uint8Array {
  const total = payload.length + 2;  // opcode + CRC8 + payload
  const frame = new Uint8Array(3 + payload.length + 1);
  frame[0] = SOF;
  frame[1] = (total >> 8) & 0xFF;
  frame[2] = total & 0xFF;
  frame[3] = opcode;
  frame.set(payload, 4);
  frame[frame.length - 1] = crc8(frame.subarray(3, 3 + payload.length + 1), payload.length + 1);
  return frame;
}

export function decodeFrame(buffer: Uint8Array): BleFrame | null {
  if (buffer.length < 5) return null;
  if (buffer[0] !== SOF) return null;
  const total = (buffer[1] << 8) | buffer[2];
  if (buffer.length < 3 + total) return null;
  const opcode = buffer[3];
  const payloadLen = total - 2;  // minus opcode and CRC8
  const payload = buffer.subarray(4, 4 + payloadLen);
  // Verify CRC
  const expectedCrc = crc8(buffer.subarray(3, 3 + payloadLen + 1), payloadLen + 1);
  if (expectedCrc !== buffer[3 + payloadLen + 1]) return null;
  return { opcode, payload: new Uint8Array(payload) };
}

// ---- Status parsing ----
import { DeviceStatus, OpMode, SkewProfile } from '../types';

const MODE_NAMES: Record<number, OpMode> = {
  0: 'inline', 1: 'rogue_gm', 2: 'passive', 3: 'transparent',
};

const SKEW_NAMES: Record<number, SkewProfile> = {
  0: 'step', 1: 'ramp', 2: 'stealth', 3: 'jitter', 4: 'sawtooth',
};

export function parseStatus(payload: Uint8Array): DeviceStatus | null {
  if (payload.length < 26) return null;
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const skewNsLo = dv.getInt32(4, true);
  const skewNsHi = dv.getInt32(8, true);
  // Reconstruct int64 (may lose precision for very large values)
  const skewNs = skewNsHi * 0x100000000 + (skewNsLo >>> 0);
  const skewStr = `${skewNsHi >= 0 ? '' : '-'}${Math.abs(skewNs).toLocaleString()}`;
  return {
    connected: true,
    mode: MODE_NAMES[payload[1]] ?? 'passive',
    skewActive: payload[2] !== 0,
    skewProfile: SKEW_NAMES[payload[3]] ?? 'step',
    currentSkewNs: skewNs,
    currentSkewNsStr: skewStr,
    observedGmPriority1: payload[12],
    observedGmClockClass: payload[13],
    spoofedGmPriority1: payload[14],
    spoofedGmClockClass: payload[15],
    framesCaptured: dv.getUint32(16, true),
    framesModified: dv.getUint32(20, true),
    ntpRequests: payload[24],
    ntpResponses: payload[25],
    batteryMv: 4000,  // placeholder — not in status payload
    temperatureC: 25,
    gnssLocked: false,
    bleRssi: -60,
  };
}

// ---- Skew config encoding ----
import { SkewConfig, SkewPreset } from '../types';

const SKEW_PROFILE_CODES: Record<SkewProfile, number> = {
  step: 0, ramp: 1, stealth: 2, jitter: 3, sawtooth: 4,
};

const SKEW_PRESET_CODES: Record<SkewPreset, number> = {
  kerberos_ext: 0,
  kerberos_replay: 1,
  pmu_slow: 2,
  pmu_sawtooth: 3,
  jitter_100us: 4,
  custom: 0xFF,
};

export function encodeSkewConfig(cfg: SkewConfig): Uint8Array {
  const buf = new Uint8Array(26);
  const dv = new DataView(buf.buffer);
  buf[0] = SKEW_PROFILE_CODES[cfg.profile];
  dv.setInt32(1, cfg.offsetNs & 0xFFFFFFFF, true);
  dv.setInt32(5, (cfg.offsetNs / 0x100000000) & 0xFFFFFFFF, true);
  dv.setInt32(9, cfg.rateNsps & 0xFFFFFFFF, true);
  dv.setInt32(13, (cfg.rateNsps / 0x100000000) & 0xFFFFFFFF, true);
  dv.setUint32(17, cfg.jitterAmpNs, true);
  dv.setUint32(21, cfg.sawtoothPeriodMs, true);
  buf[25] = cfg.active ? 1 : 0;
  return buf;
}

export function encodePreset(preset: SkewPreset): Uint8Array {
  const buf = new Uint8Array(1);
  buf[0] = SKEW_PRESET_CODES[preset];
  return buf;
}

// ---- BMCA config encoding ----
import { BmcaConfig } from '../types';

export function encodeBmcaConfig(cfg: BmcaConfig): Uint8Array {
  const buf = new Uint8Array(7);
  buf[0] = cfg.priority1;
  buf[1] = cfg.clockClass;
  buf[2] = cfg.clockAccuracy;
  buf[3] = (cfg.clockVariance >> 8) & 0xFF;
  buf[4] = cfg.clockVariance & 0xFF;
  buf[5] = cfg.priority2;
  buf[6] = cfg.domainNumber;
  return buf;
}

// ---- Mode encoding ----
const MODE_CODES: Record<OpMode, number> = {
  inline: 0, rogue_gm: 1, passive: 2, transparent: 3,
};

export function encodeMode(mode: OpMode): Uint8Array {
  const buf = new Uint8Array(1);
  buf[0] = MODE_CODES[mode];
  return buf;
}

// ---- Covert data encoding ----
export function encodeCovertData(text: string): Uint8Array {
  const enc = new TextEncoder();
  return enc.encode(text);
}

// ---- Hex helpers ----
export function bytesToHex(bytes: Uint8Array, maxBytes: number = 32): string {
  const n = Math.min(bytes.length, maxBytes);
  let hex = '';
  for (let i = 0; i < n; i++) {
    hex += bytes[i].toString(16).padStart(2, '0') + ' ';
  }
  if (bytes.length > maxBytes) hex += `... (${bytes.length} bytes)`;
  return hex.trim();
}

export function hexToBytes(hex: string): Uint8Array {
  const clean = hex.replace(/\s+/g, '').replace(/0x/gi, '');
  const bytes = new Uint8Array(clean.length / 2);
  for (let i = 0; i < clean.length; i += 2) {
    bytes[i / 2] = parseInt(clean.substr(i, 2), 16);
  }
  return bytes;
}

// Author: jayis1
// License: GPL-2.0