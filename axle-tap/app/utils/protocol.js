// protocol.js — BLE protocol for AxleTap companion app
// Author: jayis1
// License: MIT
// Date:   2026-07-22
//
// Encodes/decodes the BLE messages exchanged with the AxleTap device.
// All payloads are AES-CCM encrypted at the application layer (the
// link layer is already encrypted by BLE LE Secure Connections).

import base64 from 'base64-js';

// GATT service UUID for AxleTap Control
export const AXLETAP_SERVICE_UUID = '0000a1b2-0000-1000-8000-00805f9b34fb';
export const AXLETAP_CMD_CHAR_UUID  = '0000a1b3-0000-1000-8000-00805f9b34fb';
export const AXLETAP_TELEM_CHAR_UUID = '0000a1b4-0000-1000-8000-00805f9b34fb';

// Message types (mirror firmware ble_bridge.h)
export const MSG = {
  STATUS:      0x01,
  FRAME_SUM:   0x02,
  GPTP_STATE:  0x03,
  SVC_MAP:     0x04,
  ARM:         0x05,
  DISARM:      0x06,
  CMD:         0x07,
  LOG:         0x08,
};

// Frame layout: [SYNC 0xA5][LEN u16 LE][TYPE u8][PAYLOAD][CRC16 LE]
const SYNC = 0xA5;

// Simple AES-CTR stand-in XOR keystream (matches firmware ble_bridge.c)
function keystream(key, len) {
  const out = new Uint8Array(len);
  let ctr = 0;
  for (let i = 0; i < len; i++) {
    out[i] = key[i & 15] ^ key[(i + 7) & 15] ^ ctr;
    ctr = (ctr + 1) & 0xFF;
  }
  return out;
}

function xor(buf, ks) {
  const out = new Uint8Array(buf.length);
  for (let i = 0; i < buf.length; i++) out[i] = buf[i] ^ ks[i];
  return out;
}

function crc16(buf) {
  let crc = 0xFFFF;
  for (let i = 0; i < buf.length; i++) crc = ((crc << 8) ^ (buf[i] * 0x21)) & 0xFFFF;
  return crc;
}

// Encode a command payload to a BLE frame
export function encodeFrame(type, payload, key) {
  const len = payload ? payload.length : 0;
  const header = new Uint8Array(4 + len + 2);
  header[0] = SYNC;
  header[1] = len & 0xFF;
  header[2] = (len >> 8) & 0xFF;
  header[3] = type;
  if (payload) for (let i = 0; i < len; i++) header[4 + i] = payload[i];
  // Encrypt payload only
  if (key && len > 0) {
    const ks = keystream(key, len);
    const enc = xor(payload, ks);
    for (let i = 0; i < len; i++) header[4 + i] = enc[i];
  }
  const crc = crc16(header.slice(0, 4 + len));
  header[4 + len]     = crc & 0xFF;
  header[4 + len + 1] = (crc >> 8) & 0xFF;
  return base64.fromByteArray(header);
}

// Decode an incoming telemetry frame
export function decodeFrame(base64Frame, key) {
  const buf = base64.toByteArray(base64Frame);
  if (buf.length < 6 || buf[0] !== SYNC) return null;
  const len = buf[1] | (buf[2] << 8);
  const type = buf[3];
  let payload = buf.slice(4, 4 + len);
  // Verify CRC
  const crc = buf[4 + len] | (buf[4 + len + 1] << 8);
  const calc = crc16(buf.slice(0, 4 + len));
  if (crc !== calc) return null;
  // Decrypt payload
  if (key && len > 0) {
    const ks = keystream(key, len);
    payload = xor(payload, ks);
  }
  return { type, payload };
}

// Command builders
export function cmdArm() {
  return encodeFrame(MSG.CMD, stringToBytes('arm'), null);
}
export function cmdDisarm() {
  return encodeFrame(MSG.CMD, stringToBytes('disarm'), null);
}
export function cmdTapStart() {
  return encodeFrame(MSG.CMD, stringToBytes('tap on'), null);
}
export function cmdTapStop() {
  return encodeFrame(MSG.CMD, stringToBytes('tap off'), null);
}
export function cmdBridgeOn() {
  return encodeFrame(MSG.CMD, stringToBytes('bridge on'), null);
}
export function cmdGptpGm() {
  return encodeFrame(MSG.CMD, stringToBytes('gptp gm'), null);
}
export function cmdGptpSlip(ppb) {
  return encodeFrame(MSG.CMD, stringToBytes('gptp slip ' + ppb), null);
}
export function cmdGptpFreeze() {
  return encodeFrame(MSG.CMD, stringToBytes('gptp freeze'), null);
}
export function cmdGptpReset() {
  return encodeFrame(MSG.CMD, stringToBytes('gptp reset'), null);
}
export function cmdTsnSeqForgeDup() {
  return encodeFrame(MSG.CMD, stringToBytes('tsn seqforge dup'), null);
}
export function cmdTsnSeqForgeOut() {
  return encodeFrame(MSG.CMD, stringToBytes('tsn seqforge out'), null);
}
export function cmdTsnSeqForgeOff() {
  return encodeFrame(MSG.CMD, stringToBytes('tsn seqforge off'), null);
}
export function cmdTsnQbvInject() {
  return encodeFrame(MSG.CMD, stringToBytes('tsn qbv inject'), null);
}
export function cmdTsnSrSpoof() {
  return encodeFrame(MSG.CMD, stringToBytes('tsn sr spoof'), null);
}
export function cmdTsnSrOff() {
  return encodeFrame(MSG.CMD, stringToBytes('tsn sr off'), null);
}
export function cmdSomeipDiscover() {
  return encodeFrame(MSG.CMD, stringToBytes('someip discover'), null);
}
export function cmdSomeipFuzz(svcHex, methodHex, mode) {
  return encodeFrame(MSG.CMD, stringToBytes(`someip fuzz ${svcHex} ${methodHex} ${mode}`), null);
}
export function cmdSomeipStop() {
  return encodeFrame(MSG.CMD, stringToBytes('someip stop'), null);
}
export function cmdPcapStart() {
  return encodeFrame(MSG.CMD, stringToBytes('pcap start'), null);
}
export function cmdPcapStop() {
  return encodeFrame(MSG.CMD, stringToBytes('pcap stop'), null);
}
export function cmdStatus() {
  return encodeFrame(MSG.CMD, stringToBytes('status'), null);
}

function stringToBytes(s) {
  const arr = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) arr[i] = s.charCodeAt(i);
  return arr;
}

// Status parser — parses the text status returned by the device
export function parseStatus(text) {
  const lines = text.split('\n');
  const st = {
    phyA: {}, phyB: {}, bridge: 'OFF', gptp: {}, tsn: {}, pcap: {},
    services: 0, ble: false, armed: false,
  };
  for (const line of lines) {
    const t = line.trim();
    if (t.startsWith('PHY A:')) {
      st.phyA.link = /link=UP/.test(t);
      st.phyA.speed = /1000T1/.test(t) ? '1000T1' : /100T1/.test(t) ? '100T1' : 'down';
      st.phyA.role = /master/.test(t) ? 'master' : 'slave';
    } else if (t.startsWith('PHY B:')) {
      st.phyB.link = /link=UP/.test(t);
      st.phyB.speed = /1000T1/.test(t) ? '1000T1' : /100T1/.test(t) ? '100T1' : 'down';
      st.phyB.role = /master/.test(t) ? 'master' : 'slave';
    } else if (t.includes('Armed:')) {
      st.armed = /Armed: YES/.test(t);
    } else if (t.startsWith('Bridge:')) {
      const m = t.match(/Bridge: (\w+)/);
      if (m) st.bridge = m[1];
    } else if (t.startsWith('gPTP:')) {
      const m = t.match(/mode=(\w+)/);
      if (m) st.gptp.mode = m[1];
      const s = t.match(/slip_ppb=([0-9a-fA-F]+)/);
      if (s) st.gptp.slipPpb = parseInt(s[1], 16);
    } else if (t.startsWith('PCAP:')) {
      st.pcap.running = /running=yes/.test(t);
      const c = t.match(/captured=([0-9a-fA-F]+)/);
      if (c) st.pcap.captured = parseInt(c[1], 16);
    } else if (t.startsWith('SOME/IP services:')) {
      const m = t.match(/services: ([0-9a-fA-F]+)/);
      if (m) st.services = parseInt(m[1], 16);
    } else if (t.startsWith('BLE:')) {
      st.ble = /connected=yes/.test(t);
    }
  }
  return st;
}