// Credential Canary wire protocol and event decoder
// Author: jayis1
// SPDX-License-Identifier: GPL-2.0-only
export const SOF = 0xca;
export const Command = {
  INFO: 1,
  STATUS: 2,
  START: 3,
  STOP: 4,
  MODE: 5,
  MARK: 6,
  RESET: 7,
};
export const Mode = { SAFE_BYPASS: 0, MONITOR: 1, BRIDGE: 2, LAB_TEST: 3 };
export const EventType = {
  1: "Boot",
  2: "Wiegand bit",
  3: "Wiegand frame",
  4: "OSDP frame",
  5: "Tamper",
  6: "Fault",
  7: "Marker",
  8: "Policy alert",
};
export const Interface = {
  0: "System",
  1: "Wiegand reader",
  2: "Wiegand panel",
  3: "OSDP reader",
  4: "OSDP panel",
};
export const Alert = {
  1: "Parity failure",
  2: "Replay-window match",
  3: "Plaintext OSDP credential",
  4: "OSDP address changed",
  5: "Malformed frame",
  6: "Line stuck",
  7: "Enclosure tamper",
};

export function crc16(bytes) {
  let crc = 0xffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let b = 0; b < 8; b++)
      crc = crc & 1 ? (crc >>> 1) ^ 0xa001 : crc >>> 1;
  }
  return crc & 0xffff;
}
export function packet(command, payload = []) {
  if (payload.length > 58) throw new Error("payload too long");
  const body = [SOF, command, payload.length, ...payload];
  const crc = crc16(body);
  return Uint8Array.from([...body, crc & 255, crc >>> 8]);
}
export function parsePacket(bytes) {
  const b = Array.from(bytes);
  if (b.length < 5 || b[0] !== SOF) throw new Error("bad frame");
  if (b.length !== b[2] + 5) throw new Error("bad length");
  const expected = b[b.length - 2] | (b[b.length - 1] << 8);
  if (crc16(b.slice(0, -2)) !== expected) throw new Error("bad crc");
  return { type: b[1], payload: b.slice(3, -2) };
}
export function decodeEvent(payload) {
  if (payload.length < 10) throw new Error("short event");
  const sequence = payload[0] | (payload[1] << 8);
  const timestampUs =
    (payload[2] |
      (payload[3] << 8) |
      (payload[4] << 16) |
      (payload[5] << 24)) >>>
    0;
  const type = payload[6],
    iface = payload[7],
    flags = payload[8],
    length = payload[9];
  if (payload.length !== 10 + length) throw new Error("event length");
  const data = payload.slice(10);
  const decoded = {
    sequence,
    timestampUs,
    type,
    typeName: EventType[type] || "Unknown",
    interface: Interface[iface] || `Interface ${iface}`,
    flags,
    data,
  };
  if (type === 3 && data.length >= 12) {
    decoded.bits = data[0];
    decoded.parityOk = !!data[1];
    decoded.glitches = data[3];
    decoded.credential = data
      .slice(4, 12)
      .map((v) => v.toString(16).padStart(2, "0"))
      .join("");
  } else if (type === 4 && data.length >= 8) {
    decoded.address = data[0];
    decoded.command = `0x${data[2].toString(16).padStart(2, "0")}`;
    decoded.secure = !!data[3];
    decoded.valid = !!data[4];
  } else if (type === 8 && data.length)
    decoded.alert = Alert[data[0]] || `Alert ${data[0]}`;
  else if (type === 7) decoded.marker = String.fromCharCode(...data);
  return decoded;
}
export function statusFromPayload(p) {
  if (p.length < 16) throw new Error("short status");
  const u32 = (i) =>
    (p[i] | (p[i + 1] << 8) | (p[i + 2] << 16) | (p[i + 3] << 24)) >>> 0;
  return {
    mode: p[0],
    streaming: !!p[1],
    tamper: !!p[2],
    uptimeMs: u32(4),
    alerts: u32(8),
    drops: p[12] | (p[13] << 8),
    queued: p[14] | (p[15] << 8),
  };
}
