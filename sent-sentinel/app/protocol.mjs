/* SENT Sentinel framed USB protocol
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
export const MAGIC = 0x5353;
export const VERSION = 1;
export const MessageType = Object.freeze({
  STATUS: 1,
  EVENT: 2,
  BASELINE: 3,
  COMMAND: 16,
});

export function crc16(bytes) {
  let crc = 0xffff;
  for (const byte of bytes) {
    crc ^= byte << 8;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = ((crc & 0x8000) !== 0 ? (crc << 1) ^ 0x1021 : crc << 1) & 0xffff;
    }
  }
  return crc;
}

export function encodeMessage(type, sequence, payload = new Uint8Array()) {
  if (!(payload instanceof Uint8Array) || payload.length > 4096) throw new RangeError('invalid payload');
  if (!Number.isInteger(type) || type < 0 || type > 255) throw new RangeError('invalid type');
  const bytes = new Uint8Array(10 + payload.length);
  const view = new DataView(bytes.buffer);
  view.setUint16(0, MAGIC, false);
  view.setUint8(2, VERSION);
  view.setUint8(3, type);
  view.setUint16(4, sequence & 0xffff, false);
  view.setUint16(6, payload.length, false);
  bytes.set(payload, 8);
  view.setUint16(8 + payload.length, crc16(bytes.subarray(0, 8 + payload.length)), false);
  return bytes;
}

export function decodeMessage(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < 10) throw new Error('short message');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (view.getUint16(0, false) !== MAGIC) throw new Error('bad magic');
  if (view.getUint8(2) !== VERSION) throw new Error('unsupported version');
  const length = view.getUint16(6, false);
  if (length > 4096 || bytes.length !== length + 10) throw new Error('bad length');
  const expected = view.getUint16(8 + length, false);
  const observed = crc16(bytes.subarray(0, 8 + length));
  if (expected !== observed) throw new Error('bad crc');
  return {
    type: view.getUint8(3),
    sequence: view.getUint16(4, false),
    payload: bytes.slice(8, 8 + length),
  };
}

export function parseEvent(text) {
  const event = JSON.parse(text);
  const integers = ['channel', 'timestamp_us', 'value', 'analog_mv', 'score', 'flags'];
  for (const key of integers) {
    if (!Number.isSafeInteger(event[key]) || event[key] < 0) throw new Error(`invalid ${key}`);
  }
  if (event.channel > 3 || event.value > 4095 || event.analog_mv > 5000 || event.score > 100) {
    throw new Error('event out of range');
  }
  if (typeof event.kind !== 'string' || event.kind.length > 48) throw new Error('invalid kind');
  return Object.freeze({ ...event });
}

export function buildCommand(action) {
  const allowed = new Set(['stage-learn', 'stage-freeze', 'export-summary', 'clear-staged']);
  if (!allowed.has(action)) throw new Error('command not allowed');
  return new TextEncoder().encode(JSON.stringify({ author: 'jayis1', action }));
}
