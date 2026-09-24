/* FlexRay Sentinel app protocol tests
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import test from 'node:test';
import assert from 'node:assert/strict';
import { MessageType, buildConfig, crc16, decodeEventPayload, decodeMessage, encodeMessage } from './protocol.mjs';

test('message round trip preserves metadata and payload', () => {
  const payload = new Uint8Array([1, 2, 3, 4]);
  const decoded = decodeMessage(encodeMessage(MessageType.CONFIG, 42, payload));
  assert.equal(decoded.type, MessageType.CONFIG);
  assert.equal(decoded.sequence, 42);
  assert.deepEqual([...decoded.payload], [...payload]);
});

test('CRC detects corruption', () => {
  const encoded = encodeMessage(MessageType.STATUS, 1, new Uint8Array([9]));
  encoded[10] ^= 0x80;
  assert.throws(() => decodeMessage(encoded), /CRC/);
});

test('configuration validates safe bounds', () => {
  assert.deepEqual([...buildConfig({ threshold: 75, toleranceTicks: 256 })], [75, 1, 0, 0]);
  assert.throws(() => buildConfig({ threshold: 0 }), /threshold/);
});

test('event JSON is validated', () => {
  const bytes = new TextEncoder().encode('{"slot":44,"score":71,"kind":"timing"}');
  assert.equal(decodeEventPayload(bytes).slot, 44);
});

test('CRC has stable reference vector', () => {
  assert.equal(crc16(new Uint8Array([1, 2, 3, 4])), 0x89c3);
});
