// OneWire Cartographer wire protocol. Author: jayis1. SPDX-License-Identifier: MIT
export const MAGIC = 0x4f574331;
export const COMMAND = Object.freeze({
  INFO: 0x01, STATUS: 0x02, START: 0x10, STOP: 0x11,
  READ: 0x12, SCAN: 0x20, THRESHOLDS: 0x21, ARM: 0x30,
  RESET: 0x31, BYTE: 0x32, MODE: 0x33, CLEAR_FAULT: 0x40,
});

export function frame(command, payload = new Uint8Array(), sequence = 1) {
  if (payload.length > 244) throw new RangeError('payload exceeds device frame');
  const bytes = new Uint8Array(12 + payload.length);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, MAGIC, true);
  view.setUint8(4, 1);
  view.setUint8(5, command);
  view.setUint16(6, payload.length, true);
  view.setUint32(8, sequence, true);
  bytes.set(payload, 12);
  return bytes;
}

export function parseEvent(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length < 16) throw new Error('short event');
  const v = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    type: v.getUint8(0), port: v.getUint8(1), value: v.getUint8(2), flags: v.getUint8(3),
    timestamp: v.getUint32(4, true), duration: v.getUint32(8, true), millivolts: v.getUint16(12, true),
  };
}

export function expectedResponse(challenge) {
  let value = (challenge ^ 0x4a415931) >>> 0;
  value = (value ^ (value << 13)) >>> 0;
  value = (value ^ (value >>> 17)) >>> 0;
  value = (value ^ (value << 5)) >>> 0;
  return (value ^ 0xa5c35a7d) >>> 0;
}

export function romText(rom) {
  return [...rom].map(value => value.toString(16).padStart(2, '0')).join('-').toUpperCase();
}
