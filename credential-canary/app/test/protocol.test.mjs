// Credential Canary protocol tests
// Author: jayis1
// SPDX-License-Identifier: GPL-2.0-only
import test from "node:test";
import assert from "node:assert/strict";
import {
  packet,
  parsePacket,
  decodeEvent,
  statusFromPayload,
  Command,
} from "../src/protocol.js";
test("command packet round trips", () => {
  const raw = packet(Command.MODE, [2]);
  assert.deepEqual(parsePacket(raw), { type: Command.MODE, payload: [2] });
});
test("corruption is rejected", () => {
  const raw = packet(Command.INFO);
  raw[1] ^= 1;
  assert.throws(() => parsePacket(raw), /crc/);
});
test("policy event is decoded", () => {
  const p = [7, 0, 64, 66, 15, 0, 8, 3, 1, 1, 3];
  const e = decodeEvent(p);
  assert.equal(e.sequence, 7);
  assert.equal(e.alert, "Plaintext OSDP credential");
});
test("status fields decode", () => {
  const s = statusFromPayload([
    1, 1, 0, 0, 232, 3, 0, 0, 2, 0, 0, 0, 0, 0, 4, 0,
  ]);
  assert.equal(s.uptimeMs, 1000);
  assert.equal(s.queued, 4);
});
