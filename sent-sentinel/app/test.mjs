/* SENT Sentinel protocol tests
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import test from 'node:test';
import assert from 'node:assert/strict';
import { MessageType, buildCommand, decodeMessage, encodeMessage, parseEvent } from './protocol.mjs';

test('message round trip preserves metadata and payload', () => {
  const payload = new TextEncoder().encode('jayis1:SENT');
  const decoded = decodeMessage(encodeMessage(MessageType.EVENT, 513, payload));
  assert.equal(decoded.type, MessageType.EVENT);
  assert.equal(decoded.sequence, 513);
  assert.equal(new TextDecoder().decode(decoded.payload), 'jayis1:SENT');
});

test('corruption is rejected', () => {
  const message = encodeMessage(MessageType.STATUS, 1, new Uint8Array([1, 2, 3]));
  message[8] ^= 0x40;
  assert.throws(() => decodeMessage(message), /crc/);
});

test('events are bounded', () => {
  const valid = parseEvent(JSON.stringify({
    channel: 2, timestamp_us: 9000, value: 2048, analog_mv: 2500,
    score: 76, flags: 9, kind: 'analog-mismatch',
  }));
  assert.equal(valid.channel, 2);
  assert.throws(() => parseEvent(JSON.stringify({ ...valid, channel: 8 })), /range/);
});

test('only passive workflow commands are accepted', () => {
  assert.match(new TextDecoder().decode(buildCommand('stage-freeze')), /jayis1/);
  assert.throws(() => buildCommand('inject-frame'), /not allowed/);
});
