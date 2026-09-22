// RFFE Sentinel application tests
// Author: jayis1
// SPDX-License-Identifier: MIT
import test from 'node:test';
import assert from 'node:assert/strict';
import { crc32c, filterEvents, validateRule, MAGIC, VERSION } from './app.js';

test('protocol constants identify RSCP/1', () => {
  assert.equal(MAGIC, 0x50435352);
  assert.equal(VERSION, 1);
});

test('CRC-32C matches the standard check vector', () => {
  assert.equal(crc32c(new TextEncoder().encode('123456789')), 0xe3069283);
});

test('policy rejects unsafe and malformed ranges', () => {
  assert.match(validateRule({ usid: 16, first: 0, last: 1, action: 'alert', delay: 0 }), /USID/);
  assert.match(validateRule({ usid: 2, first: 8, last: 1, action: 'deny', delay: 0 }), /range/);
  assert.match(validateRule({ usid: 2, first: 0, last: 1, action: 'delay', delay: 3000 }), /Delay/);
  assert.equal(validateRule({ usid: 2, first: 0x10, last: 0x1f, action: 'deny', delay: 0 }), '');
});

test('event filter searches all visible fields', () => {
  const events = [
    { usid: 2, action: 'forward', register: '0x10' },
    { usid: 7, action: 'alert', register: '0x22' }
  ];
  assert.equal(filterEvents(events, 'alert').length, 1);
  assert.equal(filterEvents(events, '0x10')[0].usid, 2);
  assert.equal(filterEvents(events, '').length, 2);
});
