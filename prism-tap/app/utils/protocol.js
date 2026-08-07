/**
 * utils/protocol.js — BLE Command Protocol for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements the binary command/response protocol for communicating with
 * the Prism-Tap device over BLE.
 */

// Command opcodes (must match firmware/drivers/ble_if.h)
export const CMD = {
  PING: 0x01,
  GET_STATUS: 0x02,
  SET_MODE: 0x03,
  START_CAPTURE: 0x10,
  STOP_CAPTURE: 0x11,
  GET_FRAME_THUMB: 0x12,
  GET_FRAME_LIST: 0x13,
  LOAD_INJECT: 0x20,
  START_INJECT: 0x21,
  STOP_INJECT: 0x22,
  SET_INJECT_MODE: 0x23,
  SET_TIMING: 0x30,
  DROP_FRAMES: 0x31,
  GET_CONFIG: 0x40,
  SET_CONFIG: 0x41,
  ERASE_FRAMES: 0x50,
  EXPORT_FRAMES: 0x51,
  FW_UPDATE: 0x60,
};

// Operating modes (must match firmware/board.h)
export const MODE = {
  IDLE: 0,
  PASSTHROUGH: 1,
  CAPTURE_ONLY: 2,
  INJECT_ONLY: 3,
  FULL_MITM: 4,
  CAPTURE_LOW_POWER: 5,
  DIAGNOSTIC: 6,
};

export const MODE_NAMES = {
  0: 'IDLE',
  1: 'PASSTHROUGH',
  2: 'CAPTURE ONLY',
  3: 'INJECT ONLY',
  4: 'FULL MITM',
  5: 'LOW POWER CAPTURE',
  6: 'DIAGNOSTIC',
};

// Frame formats (must match firmware/board.h)
export const FMT = {
  RAW8: 0,
  RAW10: 1,
  RAW12: 2,
  YUV422: 3,
  RGB888: 4,
  JPEG: 5,
};

export const FMT_NAMES = {
  0: 'RAW8',
  1: 'RAW10',
  2: 'RAW12',
  3: 'YUV422',
  4: 'RGB888',
  5: 'JPEG',
};

// Injection modes (must match firmware/board.h)
export const INJECT_MODE = {
  FULL_REPLACE: 0,
  OVERLAY: 1,
  SELECTIVE: 2,
};

// Build a command payload for SET_MODE
export const buildSetModePayload = (mode) => {
  return [mode & 0xFF];
};

// Build a command payload for START_CAPTURE
export const buildStartCapturePayload = (format, width, height, intervalMs, maxFrames, jpegCompress) => {
  return [
    format & 0xFF,
    width & 0xFF,
    (width >> 8) & 0xFF,
    height & 0xFF,
    (height >> 8) & 0xFF,
    intervalMs & 0xFF,
    (intervalMs >> 8) & 0xFF,
    (intervalMs >> 16) & 0xFF,
    (intervalMs >> 24) & 0xFF,
    maxFrames & 0xFF,
    (maxFrames >> 8) & 0xFF,
    (maxFrames >> 16) & 0xFF,
    (maxFrames >> 24) & 0xFF,
    jpegCompress ? 1 : 0,
  ];
};

// Build a command payload for START_INJECT
export const buildStartInjectPayload = (mode, overlayX, overlayY, overlayW, overlayH, triggerInterval) => {
  return [
    mode & 0xFF,
    overlayX & 0xFF,
    (overlayX >> 8) & 0xFF,
    overlayY & 0xFF,
    (overlayY >> 8) & 0xFF,
    overlayW & 0xFF,
    (overlayW >> 8) & 0xFF,
    overlayH & 0xFF,
    (overlayH >> 8) & 0xFF,
    triggerInterval & 0xFF,
    (triggerInterval >> 8) & 0xFF,
    (triggerInterval >> 16) & 0xFF,
    (triggerInterval >> 24) & 0xFF,
  ];
};

// Build a command payload for SET_TIMING
export const buildSetTimingPayload = (enable, delayNs, jitterPct, dropPattern) => {
  const delay = delayNs | 0;  // ensure integer
  return [
    enable ? 1 : 0,
    delay & 0xFF,
    (delay >> 8) & 0xFF,
    (delay >> 16) & 0xFF,
    (delay >> 24) & 0xFF,
    jitterPct & 0xFF,
    dropPattern & 0xFF,
  ];
};

// Build a command payload for DROP_FRAMES
export const buildDropFramesPayload = (nFrames) => {
  return [
    nFrames & 0xFF,
    (nFrames >> 8) & 0xFF,
  ];
};

// Parse a status response (21 bytes)
export const parseStatusResponse = (data) => {
  if (!data || data.length < 21) return null;

  return {
    mode: data[0],
    batteryPct: data[1],
    batteryMv: (data[2] << 8) | data[3],
    sdPresent: data[4] === 1,
    bleConnected: data[5] === 1,
    usbConnected: data[6] === 1,
    fpgaReady: data[7] === 1,
    csiLinkUp: data[8] === 1,
    dsiLinkUp: data[9] === 1,
    uptime: (data[10] << 24) | (data[11] << 16) | (data[12] << 8) | data[13],
    framesCaptured: (data[14] << 24) | (data[15] << 16) | (data[16] << 8) | data[17],
    injectActive: data[18] === 1,
    framesInjected: (data[19] << 8) | data[20],
  };
};

// Parse a ping response (4 bytes: status + major + minor + patch)
export const parsePingResponse = (data) => {
  if (!data || data.length < 4) return null;
  return {
    status: data[0],
    major: data[1],
    minor: data[2],
    patch: data[3],
    version: `${data[1]}.${data[2]}.${data[3]}`,
  };
};

// CRC-16-CCITT
export const crc16 = (data) => {
  let crc = 0xFFFF;
  for (const byte of data) {
    crc ^= (byte & 0xFF) << 8;
    for (let i = 0; i < 8; i++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc & 0xFFFF;
};

// Build a complete BLE command packet
export const buildPacket = (opcode, payload = [], seq = 0) => {
  const length = payload.length;
  const crcData = [opcode, seq, length & 0xFF, (length >> 8) & 0xFF, ...payload];
  const crc = crc16(crcData);

  return [
    opcode,
    seq,
    length & 0xFF,
    (length >> 8) & 0xFF,
    crc & 0xFF,
    (crc >> 8) & 0xFF,
    ...payload,
  ];
};

export default { CMD, MODE, FMT, INJECT_MODE };