// DALI Sentinel protocol tests
// Author: jayis1
// Copyright (c) 2026 jayis1
import test from 'node:test';
import assert from 'node:assert/strict';
function crc32c(data) {
  let crc = 0xffffffff;
  for (const byte of data) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ ((crc & 1) ? 0x82f63b78 : 0);
  }
  return (~crc) >>> 0;
}
function envelope(type, sequence, payload) {
  const out = new Uint8Array(6 + payload.length + 4);
  const view = new DataView(out.buffer);
  out[0] = 1; out[1] = type;
  view.setUint16(2, sequence, true);
  view.setUint16(4, payload.length, true);
  out.set(payload, 6);
  view.setUint32(out.length - 4, crc32c(out.slice(0, -4)), true);
  return out;
}
test('CRC-32C standard vector', () => assert.equal(crc32c(new TextEncoder().encode('123456789')), 0xe3069283));
test('envelope has explicit version, type, sequence, length and valid CRC', () => {
  const packet = envelope(1, 0x1234, Uint8Array.of(0xaa, 0x55));
  const view = new DataView(packet.buffer);
  assert.equal(packet[0], 1); assert.equal(packet[1], 1);
  assert.equal(view.getUint16(2, true), 0x1234);
  assert.equal(view.getUint16(4, true), 2);
  assert.equal(view.getUint32(packet.length - 4, true), crc32c(packet.slice(0, -4)));
});
test('single-bit corruption is detected', () => {
  const packet = envelope(2, 7, Uint8Array.of(1,2,3,4));
  const expected = new DataView(packet.buffer).getUint32(packet.length - 4, true);
  packet[7] ^= 0x10;
  assert.notEqual(crc32c(packet.slice(0, -4)), expected);
});
