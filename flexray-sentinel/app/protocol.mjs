/* FlexRay Sentinel wire protocol utilities
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
export const MAGIC = 0x46525331;
export const MessageType = Object.freeze({ STATUS: 1, FRAME: 2, EVENT: 3, CONFIG: 4 });

export function crc16(bytes) {
  let crc = 0xffff;
  for (const value of bytes) {
    crc ^= value << 8;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = ((crc & 0x8000) !== 0) ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
    }
  }
  return crc;
}

export function encodeMessage(type, sequence, payload = new Uint8Array()) {
  if (!(payload instanceof Uint8Array)) throw new TypeError('payload must be Uint8Array');
  if (payload.length > 4096) throw new RangeError('payload too large');
  const output = new Uint8Array(12 + payload.length);
  const view = new DataView(output.buffer);
  view.setUint32(0, MAGIC, false);
  view.setUint8(4, type);
  view.setUint8(5, 1);
  view.setUint16(6, sequence & 0xffff, false);
  view.setUint16(8, payload.length, false);
  output.set(payload, 10);
  view.setUint16(10 + payload.length, crc16(output.subarray(0, 10 + payload.length)), false);
  return output;
}

export function decodeMessage(input) {
  const bytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (bytes.length < 12) throw new Error('short message');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (view.getUint32(0, false) !== MAGIC) throw new Error('bad magic');
  const length = view.getUint16(8, false);
  if (bytes.length !== 12 + length) throw new Error('length mismatch');
  const expected = view.getUint16(10 + length, false);
  const actual = crc16(bytes.subarray(0, 10 + length));
  if (expected !== actual) throw new Error('CRC mismatch');
  return {
    type: view.getUint8(4),
    version: view.getUint8(5),
    sequence: view.getUint16(6, false),
    payload: bytes.slice(10, 10 + length),
  };
}

export function decodeEventPayload(payload) {
  const text = new TextDecoder().decode(payload);
  const event = JSON.parse(text);
  if (!Number.isInteger(event.slot) || event.slot < 1 || event.slot > 2047) throw new Error('invalid slot');
  if (!Number.isFinite(event.score) || event.score < 0 || event.score > 100) throw new Error('invalid score');
  return event;
}

export function buildConfig({ threshold = 60, toleranceTicks = 80, storePayloads = false } = {}) {
  if (!Number.isInteger(threshold) || threshold < 1 || threshold > 100) throw new RangeError('threshold');
  if (!Number.isInteger(toleranceTicks) || toleranceTicks < 1 || toleranceTicks > 65535) throw new RangeError('tolerance');
  return new Uint8Array([
    threshold,
    toleranceTicks >>> 8,
    toleranceTicks & 0xff,
    storePayloads ? 1 : 0,
  ]);
}
