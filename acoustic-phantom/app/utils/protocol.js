/**
 * ACOUSTIC-PHANTOM — Binary Protocol Codec
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Encodes and decodes the binary GATT protocol between the companion
 * app and the ACOUSTIC-PHANTOM device. All multi-byte values are
 * little-endian. Base64 is used for BLE characteristic values.
 */

// GATT UUIDs (matching firmware ble_bridge.h)
export const SERVICE_UUID              = '0000ac01-0000-1000-8000-00805f9b34fb';
export const CMD_CHARACTERISTIC_UUID   = '0000ff01-0000-1000-8000-00805f9b34fb';
export const EVENT_CHARACTERISTIC_UUID = '0000ff02-0000-1000-8000-00805f9b34fb';
export const AUDIO_CHARACTERISTIC_UUID = '0000ff03-0000-1000-8000-00805f9b34fb';
export const STATUS_CHARACTERISTIC_UUID= '0000ff04-0000-1000-8000-00805f9b34fb';
export const CALIB_CHARACTERISTIC_UUID = '0000ff05-0000-1000-8000-00805f9b34fb';
export const FILE_CHARACTERISTIC_UUID  = '0000ff06-0000-1000-8000-00805f9b34fb';

// Command opcodes (app → device)
export const CMD = {
  SET_PROFILE:           0x01,
  ARM:                   0x02,
  DISARM:                0x03,
  START_CALIBRATION:     0x04,
  CALIBRATION_SAMPLE:    0x05,
  FINISH_CALIBRATION:    0x06,
  ENABLE_STORAGE:        0x07,
  DISABLE_STORAGE:       0x08,
  LIST_FILES:            0x09,
  DOWNLOAD_FILE:         0x0A,
  SET_BEAM:              0x0B,
  GET_STATUS:            0x0C,
  ERASE_CALIBRATION:     0x0D,
  SET_ENCRYPTION_KEY:    0x0E,
};

// Notification opcodes (device → app)
export const NOTIF = {
  EVENT:            0x81,
  AUDIO:            0x82,
  STATUS:           0x83,
  FILE_LIST:        0x84,
  FILE_CHUNK:       0x85,
  CALIB_PROGRESS:   0x86,
};

// Profile names (matching firmware)
export const PROFILES = [
  { id: 0, name: 'Keyboard',  icon: 'keyboard' },
  { id: 1, name: 'HDD',       icon: 'hdd-o' },
  { id: 2, name: 'Printer',   icon: 'print' },
  { id: 3, name: 'SMPS',      icon: 'bolt' },
  { id: 4, name: 'Relay',     icon: 'toggle-on' },
];

// Device states
export const STATES = {
  0: 'IDLE',
  1: 'ARMED',
  2: 'CAPTURING',
  3: 'CALIBRATING',
  4: 'STORAGE',
  5: 'TAMPER',
};

// Key labels (matching firmware tflm_inference.c)
export const KEY_LABELS = [
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5',
  '6','7','8','9','SPACE','ENTER','SHIFT','TAB','ESC','CTRL','ALT',
  'F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','F11','F12',
  'UP','DOWN','LEFT','RIGHT','HOME','END','PGUP','PGDN','INS','DEL',
  'BKSP','CAPS','-','=','[',']','\\',';','\'','`',',','.','/',
  'NUM0','NUM1','NUM2','NUM3','NUM4','NUM5','NUM6','NUM7','NUM8','NUM9',
  'SCROLL','PAUSE','MENU','WIN_L','WIN_R','PRTSC','SCROLL_LK','NUM_LK',
];

// ---- Encoding helpers ---------------------------------------------------

/**
 * Encode a command packet for the CMD characteristic.
 * @param {number} cmd - Command opcode
 * @param {Uint8Array|number[]} data - Payload bytes
 * @returns {string} Base64-encoded packet
 */
export function encodeCommand(cmd, data = []) {
  const payload = new Uint8Array(data);
  const len = payload.length + 1; // includes opcode byte
  const buf = new Uint8Array(3 + len + 1); // SOF + LEN(2) + data + CRC

  buf[0] = 0xA5;                        // SOF
  buf[1] = len & 0xFF;                  // LEN low
  buf[2] = (len >> 8) & 0xFF;           // LEN high
  buf[3] = cmd;                         // opcode
  buf.set(payload, 4);                  // payload

  // CRC = XOR of opcode + payload
  let crc = cmd;
  for (let i = 0; i < payload.length; i++) {
    crc ^= payload[i];
  }
  buf[buf.length - 1] = crc;

  return base64Encode(buf);
}

/**
 * Decode a status notification.
 * Format: [state(1)] [profile(1)] [event_count(4)] [battery(1)]
 */
export function decodeStatus(base64) {
  const buf = base64Decode(base64);
  if (!buf || buf.length < 7) return null;
  return {
    state: buf[0],
    stateName: STATES[buf[0]] || 'UNKNOWN',
    profile: buf[1],
    eventCount: buf[2] | (buf[3] << 8) | (buf[4] << 16) | (buf[5] << 24),
    battery: buf[6],
  };
}

/**
 * Decode an event notification.
 * Format: [class_id(1)] [conf(4)] [timestamp(4)] [inter_event(4)]
 *         [top5_id(5)] [top5_conf(20)]
 */
export function decodeEvent(base64) {
  const buf = base64Decode(base64);
  if (!buf || buf.length < 38) return null;

  const classId = buf[0];
  const confidence = readFloat32(buf, 1);
  const timestamp = readUint32(buf, 5);
  const interEvent = readUint32(buf, 9);

  const top5Ids = [];
  for (let i = 0; i < 5; i++) top5Ids.push(buf[13 + i]);

  const top5Conf = [];
  for (let i = 0; i < 5; i++) {
    top5Conf.push(readFloat32(buf, 18 + i * 4));
  }

  return {
    classId,
    label: getLabel(classId),
    confidence,
    timestamp,
    interEvent,
    top5Ids,
    top5Conf,
  };
}

/**
 * Decode an audio notification.
 * Format: [offset(2)] [count(2)] [samples(count * 2)]
 */
export function decodeAudio(base64) {
  const buf = base64Decode(base64);
  if (!buf || buf.length < 4) return null;

  const offset = buf[0] | (buf[1] << 8);
  const count = buf[2] | (buf[3] << 8);
  const samples = new Int16Array(count);

  for (let i = 0; i < count && (4 + i * 2 + 1) < buf.length; i++) {
    samples[i] = buf[4 + i * 2] | (buf[4 + i * 2 + 1] << 8);
  }

  return { offset, count, samples };
}

/**
 * Decode a file list notification.
 */
export function decodeFileList(base64) {
  const buf = base64Decode(base64);
  if (!buf || buf.length < 2) return null;

  const count = buf[0] | (buf[1] << 8);
  const files = [];
  let offset = 2;

  for (let i = 0; i < count && offset + 42 <= buf.length; i++) {
    let name = '';
    for (let j = 0; j < 32 && buf[offset + j] !== 0; j++) {
      name += String.fromCharCode(buf[offset + j]);
    }
    offset += 32;
    const size = readUint32(buf, offset);
    offset += 4;
    const timestamp = readUint32(buf, offset);
    offset += 4;
    const encrypted = buf[offset];
    offset += 1;
    files.push({ name, size, timestamp, encrypted });
  }

  return files;
}

// ---- Label lookup -------------------------------------------------------

export function getLabel(classId) {
  if (classId < KEY_LABELS.length) return KEY_LABELS[classId];
  return `KEY_${classId}`;
}

// ---- Low-level helpers --------------------------------------------------

function readFloat32(buf, offset) {
  const view = new DataView(buf.buffer, offset, 4);
  return view.getFloat32(0, true); // little-endian
}

function readUint32(buf, offset) {
  return buf[offset] | (buf[offset + 1] << 8) |
         (buf[offset + 2] << 16) | (buf[offset + 3] << 24);
}

function base64Decode(base64) {
  // react-native-ble-plx returns base64 strings
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const clean = base64.replace(/=+$/, '');
  const bytes = [];
  let buffer = 0, bits = 0;

  for (let i = 0; i < clean.length; i++) {
    const idx = chars.indexOf(clean[i]);
    if (idx === -1) continue;
    buffer = (buffer << 6) | idx;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      bytes.push((buffer >> bits) & 0xFF);
    }
  }
  return new Uint8Array(bytes);
}

function base64Encode(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  let i = 0;

  while (i < bytes.length) {
    const b1 = bytes[i++] || 0;
    const b2 = bytes[i++] || 0;
    const b3 = bytes[i++] || 0;

    const e1 = b1 >> 2;
    const e2 = ((b1 & 0x03) << 4) | (b2 >> 4);
    const e3 = ((b2 & 0x0F) << 2) | (b3 >> 6);
    const e4 = b3 & 0x3F;

    result += chars[e1] + chars[e2];
    result += (i - 1 < bytes.length + 1) ? chars[e3] : '=';
    result += (i < bytes.length + 2) ? chars[e4] : '=';
  }

  return result;
}