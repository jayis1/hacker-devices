// OneWire Cartographer protocol tests. Author: jayis1. SPDX-License-Identifier: MIT
import test from 'node:test';
import assert from 'node:assert/strict';
import { COMMAND, frame, parseEvent, expectedResponse, romText } from '../src/protocol.js';

test('frames command with little-endian header', () => {
  const output = frame(COMMAND.START, new Uint8Array([7]), 42);
  const view = new DataView(output.buffer);
  assert.equal(output.length, 13); assert.equal(view.getUint32(0, true), 0x4f574331);
  assert.equal(output[5], COMMAND.START); assert.equal(view.getUint16(6, true), 1);
  assert.equal(view.getUint32(8, true), 42); assert.equal(output[12], 7);
});

test('parses normalized firmware event', () => {
  const bytes = new Uint8Array(16); const view = new DataView(bytes.buffer);
  bytes.set([3, 1, 0xcc, 0], 0); view.setUint32(4, 9001, true); view.setUint32(8, 60, true); view.setUint16(12, 3290, true);
  assert.deepEqual(parseEvent(bytes), { type: 3, port: 1, value: 204, flags: 0, timestamp: 9001, duration: 60, millivolts: 3290 });
});

test('challenge transform and ROM formatting are stable', () => {
  assert.equal(expectedResponse(0x12345678), 0x6fd5791a);
  assert.equal(romText(new Uint8Array([1, 42, 0, 255])), '01-2A-00-FF');
  assert.throws(() => frame(1, new Uint8Array(245)), RangeError);
});
