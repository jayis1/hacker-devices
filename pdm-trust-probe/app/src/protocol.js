// PTP/1 codec
// Author: jayis1
export const MAGIC = 0x31505450;
export const commands = Object.freeze({ info: 1, status: 2, start: 3, stop: 4, policy: 5, arm: 6, pattern: 7, events: 8, reset: 9 });
export function protocolResponse(nonce) {
  let value = (nonce ^ 0x6d2b79f5) >>> 0;
  value = (value ^ (value << 13)) >>> 0;
  value = (value ^ (value >>> 17)) >>> 0;
  return (value ^ (value << 5)) >>> 0;
}
export function armPayload(nonce) {
  const payload = new Uint8Array(8); const view = new DataView(payload.buffer);
  view.setUint32(0, nonce, true); view.setUint32(4, protocolResponse(nonce), true); return payload;
}
export function patternPayload(pattern, channelMask, durationMs) {
  if (!Number.isInteger(pattern) || pattern < 0 || pattern > 3) throw new RangeError('pattern');
  if (!Number.isInteger(channelMask) || channelMask < 1 || channelMask > 15) throw new RangeError('channelMask');
  if (!Number.isInteger(durationMs) || durationMs < 1 || durationMs > 250) throw new RangeError('durationMs');
  const payload = new Uint8Array(6); const view = new DataView(payload.buffer);
  payload[0] = pattern; payload[1] = channelMask; view.setUint32(2, durationMs, true); return payload;
}
export function encodeFrame(command, sequence, payload = new Uint8Array()) {
  if (!(payload instanceof Uint8Array) || payload.length > 244) throw new RangeError('payload');
  if (!Number.isInteger(sequence) || sequence < 1) throw new RangeError('sequence');
  const bytes = new Uint8Array(12 + payload.length);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, MAGIC, true); view.setUint32(4, sequence, true);
  view.setUint16(8, payload.length, true); bytes[10] = 1; bytes[11] = command;
  bytes.set(payload, 12); return bytes;
}
export function decodeEvent(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length !== 16) throw new RangeError('event');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return { timestamp: Number(view.getBigUint64(0, true)), value: view.getUint32(8, true), detail: view.getUint16(12, true), type: bytes[14], channel: bytes[15] };
}
