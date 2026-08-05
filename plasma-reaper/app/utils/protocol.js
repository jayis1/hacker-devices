/**
 * protocol.js — PlasmaReaper binary protocol implementation
 * Author: jayis1
 * License: MIT
 *
 * Implements the framing, CRC-8, and serialization for the
 * PlasmaReaper USB CDC / BLE command protocol.
 */

// Command codes
export const CMD_PING = 0x0D;
export const CMD_GET_VERSION = 0x0E;
export const CMD_FIRE = 0x01;
export const CMD_SET_PARAMS = 0x02;
export const CMD_GET_RESULT = 0x03;
export const CMD_START_SWEEP = 0x04;
export const CMD_STOP_SWEEP = 0x05;
export const CMD_GET_WAVEFORM = 0x06;
export const CMD_SET_TRIGGER = 0x07;
export const CMD_SET_PATTERNS = 0x08;
export const CMD_GET_STATUS = 0x09;

// Response codes
export const RESP_OK = 0x80;
export const RESP_ERROR = 0x81;
export const RESP_RESULT = 0x82;
export const RESP_SWEEP_PROGRESS = 0x83;
export const RESP_SWEEP_COMPLETE = 0x84;
export const RESP_STATUS = 0x85;
export const RESP_WAVEFORM = 0x86;
export const RESP_VERSION = 0x87;

// Frame delimiters
const FRAME_START = 0xA5;
const FRAME_END = 0x5A;

/**
 * Compute CRC-8 (polynomial 0x07, init 0x00)
 */
function crc8(data) {
  let crc = 0x00;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ 0x07) & 0xFF;
      } else {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  return crc;
}

/**
 * Build a command frame.
 * @param {number} cmd - Command code
 * @param {Uint8Array|null} payload - Payload bytes
 * @returns {Uint8Array} Complete frame with delimiters and CRC
 */
export function buildFrame(cmd, payload) {
  const plen = payload ? payload.length : 0;
  const frame = new Uint8Array(5 + plen + 1);

  frame[0] = FRAME_START;
  frame[1] = cmd;
  frame[2] = plen & 0xFF;
  frame[3] = (plen >> 8) & 0xFF;

  if (payload) {
    frame.set(payload, 4);
  }

  // CRC over cmd + len + payload
  const crcData = new Uint8Array(3 + plen);
  crcData[0] = cmd;
  crcData[1] = plen & 0xFF;
  crcData[2] = (plen >> 8) & 0xFF;
  if (payload) {
    crcData.set(payload, 3);
  }
  frame[4 + plen] = crc8(crcData);
  frame[5 + plen] = FRAME_END;

  return frame;
}

/**
 * Parse a response frame.
 * @param {Uint8Array} raw - Raw bytes received from device
 * @returns {object|null} { type, data } or null if invalid
 */
export function parseFrame(raw) {
  if (!raw || raw.length < 7) return null;
  if (raw[0] !== FRAME_START) return null;
  if (raw[raw.length - 1] !== FRAME_END) return null;

  const cmd = raw[1];
  const plen = raw[2] | (raw[3] << 8);
  const payload = raw.slice(4, 4 + plen);
  const crc = raw[4 + plen];

  // Verify CRC
  const crcData = new Uint8Array(3 + plen);
  crcData[0] = cmd;
  crcData[1] = plen & 0xFF;
  crcData[2] = (plen >> 8) & 0xFF;
  crcData.set(payload, 3);

  if (crc8(crcData) !== crc) {
    console.error('CRC mismatch in response frame');
    return null;
  }

  // Parse based on response type
  switch (cmd) {
  case RESP_OK:
    return { type: RESP_OK, data: { success: payload[0] === 1 } };

  case RESP_RESULT:
    return {
      type: RESP_RESULT,
      data: {
        outcome: payload[0],
        shotsFired: payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24),
        elapsedUs: payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24),
        waveformLen: payload[9] | (payload[10] << 8),
        targetResponse: String.fromCharCode.apply(null, payload.slice(11)),
      },
    };

  case RESP_STATUS:
    return {
      type: RESP_STATUS,
      data: {
        sweepRunning: payload[0] === 1,
        totalCells: payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24),
        completedCells: payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24),
        successCount: payload[9] | (payload[10] << 8) | (payload[11] << 16) | (payload[12] << 24),
        totalShots: payload[13] | (payload[14] << 8) | (payload[15] << 16) | (payload[16] << 24),
        successShots: payload[17] | (payload[18] << 8) | (payload[19] << 16) | (payload[20] << 24),
      },
    };

  case RESP_SWEEP_PROGRESS:
    return {
      type: RESP_SWEEP_PROGRESS,
      data: {
        completedCells: payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24),
        successCount: payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24),
        outcome: payload[8],
        targetResponse: String.fromCharCode.apply(null, payload.slice(9)),
      },
    };

  case RESP_SWEEP_COMPLETE:
    return {
      type: RESP_SWEEP_COMPLETE,
      data: {
        totalCells: payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24),
        successCount: payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24),
        failureCount: payload[8] | (payload[9] << 8) | (payload[10] << 16) | (payload[11] << 24),
      },
    };

  case RESP_WAVEFORM:
    return {
      type: RESP_WAVEFORM,
      data: {
        len: payload[0] | (payload[1] << 8),
        samples: parseWaveformSamples(payload.slice(2)),
      },
    };

  case RESP_VERSION:
    return {
      type: RESP_VERSION,
      data: {
        major: payload[0],
        minor: payload[1],
        patch: payload[2],
      },
    };

  default:
    return { type: cmd, data: payload };
  }
}

function parseWaveformSamples(bytes) {
  const samples = [];
  for (let i = 0; i + 1 < bytes.length; i += 2) {
    samples.push(bytes[i] | (bytes[i + 1] << 8));
  }
  return samples;
}

/**
 * Read a BLE response (monitor the notification characteristic).
 */
export async function readBleResponse(device, serviceUuid, charUuid) {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error('BLE read timeout')), 5000);

    device.monitorCharacteristicForService(
      serviceUuid, charUuid,
      (error, characteristic) => {
        if (error) {
          clearTimeout(timeout);
          reject(error);
          return;
        }
        if (characteristic && characteristic.value) {
          clearTimeout(timeout);
          const raw = base64ToUint8Array(characteristic.value);
          resolve(raw);
        }
      }
    );
  });
}

function base64ToUint8Array(base64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const lookup = new Array(256);
  for (let i = 0; i < chars.length; i++) lookup[chars.charCodeAt(i)] = i;

  const bytes = [];
  for (let i = 0; i < base64.length; i += 4) {
    const a = lookup[base64.charCodeAt(i)] || 0;
    const b = lookup[base64.charCodeAt(i + 1)] || 0;
    const c = lookup[base64.charCodeAt(i + 2)] || 0;
    const d = lookup[base64.charCodeAt(i + 3)] || 0;
    bytes.push((a << 2) | (b >> 4));
    if (base64[i + 2] !== '=') bytes.push(((b & 0xF) << 4) | (c >> 2));
    if (base64[i + 3] !== '=') bytes.push(((c & 0x3) << 6) | d);
  }
  return new Uint8Array(bytes);
}

/**
 * Serialize glitch parameters to a Uint8Array payload.
 */
export function serializeParams(params) {
  const p = new Uint8Array(31);
  p[0] = params.vectorMask;
  writeU32(p, 1, params.triggerOffsetNs);
  writeU32(p, 5, params.glitchWidthNs);
  writeU16(p, 9, params.powerDepthMv);
  p[11] = params.powerSeriesR;
  p[12] = params.clockShape;
  writeU32(p, 13, params.clockCycleOffset);
  writeU16(p, 17, params.emPulseMv);
  writeU16(p, 19, params.emPulseWidthNs);
  writeU32(p, 21, params.interVectorNs);
  writeU16(p, 25, params.repeatCount);
  writeU32(p, 27, params.repeatDelayNs);
  return p;
}

function writeU32(arr, offset, val) {
  arr[offset] = val & 0xFF;
  arr[offset + 1] = (val >> 8) & 0xFF;
  arr[offset + 2] = (val >> 16) & 0xFF;
  arr[offset + 3] = (val >> 24) & 0xFF;
}

function writeU16(arr, offset, val) {
  arr[offset] = val & 0xFF;
  arr[offset + 1] = (val >> 8) & 0xFF;
}

/**
 * Outcome labels for display.
 */
export const OUTCOME_LABELS = {
  0: 'Pending',
  1: 'SUCCESS',
  2: 'Failure',
  3: 'No Response',
  4: 'Glitch Invalid',
  5: 'Timeout',
};

export const OUTCOME_COLORS = {
  0: '#999',
  1: '#4CAF50',
  2: '#F44336',
  3: '#FF9800',
  4: '#9C27B0',
  5: '#607D8B',
};