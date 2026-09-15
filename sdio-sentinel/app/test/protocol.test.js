// SDIO Sentinel protocol tests. Author: jayis1.
import test from 'node:test';
import assert from 'node:assert/strict';
import { decodeStatus, exportAudit, modeName, validateRule } from '../src/protocol.js';

test('decodes little-endian status payload', () => {
  const bytes = new Uint8Array(28);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, 0x53445331, true);
  view.setUint32(8, 42, true);
  view.setUint32(20, 25_000_000, true);
  bytes[24] = 1;
  bytes[26] = 1;
  bytes[27] = 1;
  const status = decodeStatus(bytes);
  assert.equal(status.boardId, 0x53445331);
  assert.equal(status.framesSeen, 42);
  assert.equal(status.clockHz, 25_000_000);
  assert.equal(status.armed, true);
  assert.equal(status.cardPresent, true);
});

test('validates bounded command rules', () => {
  assert.equal(validateRule({ command: 24, action: 'block', argumentMask: '0xffffe000' }), null);
  assert.match(validateRule({ command: 99, action: 'block', argumentMask: '0xffffffff' }), /Command/);
  assert.match(validateRule({ command: 24, action: 'drop', argumentMask: '0xffffffff' }), /action/);
  assert.match(validateRule({ command: 24, action: 'block', argumentMask: 'all' }), /mask/);
});

test('redacts raw data from audit export', () => {
  const output = exportAudit([{ sequence: 7, command: 17, rawData: 'secret-sector-bytes' }]);
  assert.doesNotMatch(output, /secret-sector-bytes/);
  assert.match(output, /jayis1/);
});

test('renders unknown modes safely', () => {
  assert.equal(modeName(1), 'Passive observe');
  assert.equal(modeName(255), 'Unknown');
});
