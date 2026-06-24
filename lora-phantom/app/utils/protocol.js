/*
 * lora-phantom / app / utils / protocol.js
 * Wire protocol codec matching firmware/drivers/protocol.c
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Frame format:
 *   [SYNC0 0x55][SYNC1 0xAA][LEN u16 LE][CMD u8][PAYLOAD[LEN]][CRC16 u16 LE]
 * CRC16-CCITT (poly 0x1021, init 0xFFFF) over CMD + PAYLOAD.
 */

export const CMD = {
  NOP:              0x00,
  PING:             0x01,
  STATUS:           0x02,
  SET_MODE:         0x03,
  SNIFF_START:      0x10,
  SNIFF_STOP:       0x11,
  SNIFF_SET_CH:     0x12,
  FRAME_RECV:       0x13,
  KEYSEARCH_RUN:    0x20,
  KEYSEARCH_DICT:   0x21,
  KEYSEARCH_RESULT: 0x22,
  REPLAY_SEND:      0x30,
  REPLAY_SET_FCNT:  0x31,
  INJECT_SEND:      0x40,
  GWEMU_START:      0x50,
  GWEMU_STOP:       0x51,
  GWEMU_SESSION:    0x52,
  FUZZ_START:       0x60,
  FUZZ_STOP:        0x61,
  SCAN_START:       0x70,
  SCAN_RESULT:      0x71,
  SET_REGION:       0x80,
  SET_TXPOWER:      0x81,
  SET_LINK_KEY:     0x82,
  SD_FORMAT:        0x90,
  OTA_BEGIN:        0xA0,
  OTA_DATA:         0xA1,
  OTA_END:          0xA2,
};

export const MODES = {
  IDLE: 0, SNIFF: 1, KEYSEARCH: 2, REPLAY: 3, INJECT: 4,
  GATEWAY_EMU: 5, FUZZ: 6, SCAN: 7,
};

export const REGIONS = {
  EU868: 0, US915: 1, AS923: 2, CN470: 3, AU915: 4, IN865: 5, KR920: 6, RU864: 7,
};

export const REGION_NAMES = {
  0: 'EU868', 1: 'US915', 2: 'AS923', 3: 'CN470',
  4: 'AU915', 5: 'IN865', 6: 'KR920', 7: 'RU864',
};

export const FUZZ_MODES = {
  BAD_HEADER_CRC: 0,
  INVALID_CR: 1,
  PHANTOM_HEADER: 2,
  IMPLICIT_MISMATCH: 3,
  OVERSIZED_PAYLOAD: 4,
  RANDOM_BYTES: 5,
};

export const SYNC0 = 0x55;
export const SYNC1 = 0xAA;
export const MAX_PAYLOAD = 512;

/* ---- CRC16-CCITT (poly 0x1021, init 0xFFFF) ---- */
export function crc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] & 0xFF) << 8;
    for (let b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      else              crc = (crc << 1) & 0xFFFF;
    }
  }
  return crc & 0xFFFF;
}

/* ---- Encode a command + payload into a framed packet (Uint8Array) ---- */
export function encode(cmd, payload) {
  const plen = payload ? payload.length : 0;
  if (plen > MAX_PAYLOAD) throw new Error('Payload too large');
  const buf = new Uint8Array(4 + 1 + plen + 2);
  buf[0] = SYNC0;
  buf[1] = SYNC1;
  buf[2] = plen & 0xFF;
  buf[3] = (plen >> 8) & 0xFF;
  buf[4] = cmd & 0xFF;
  if (payload) buf.set(payload, 5);
  const crcData = buf.slice(4, 5 + plen);
  const crc = crc16(crcData);
  buf[5 + plen] = crc & 0xFF;
  buf[5 + plen + 1] = (crc >> 8) & 0xFF;
  return buf;
}

/* ---- Decode a framed packet ---- */
export function decode(buf) {
  if (buf.length < 7) return null;
  if (buf[0] !== SYNC0 || buf[1] !== SYNC1) return null;
  const plen = buf[2] | (buf[3] << 8);
  const total = 4 + 1 + plen + 2;
  if (buf.length < total) return null;
  const crcRecv = buf[5 + plen] | (buf[5 + plen + 1] << 8);
  const crcCalc = crc16(buf.slice(4, 5 + plen));
  if (crcRecv !== crcCalc) return null;
  return {
    cmd: buf[4],
    payload: buf.slice(5, 5 + plen),
  };
}

/* ---- Helpers to build common payloads ---- */
export function buildSniffStart(freqHz, sf, bw) {
  const p = new Uint8Array(6);
  p[0] = freqHz & 0xFF;
  p[1] = (freqHz >> 8) & 0xFF;
  p[2] = (freqHz >> 16) & 0xFF;
  p[3] = (freqHz >> 24) & 0xFF;
  p[4] = sf & 0xFF;
  p[5] = bw & 0xFF;
  return p;
}

export function buildKeySearchDict(joinReqRaw, dict) {
  /* joinReqRaw: 19-byte join-request (MHDR|JoinEUI|DevEUI|DevNonce|MIC)
   * dict: array of 16-byte Uint8Array keys */
  const dictLen = dict.length;
  const p = new Uint8Array(21 + dictLen * 16);
  p.set(joinReqRaw, 0);
  p[19] = dictLen & 0xFF;
  p[20] = (dictLen >> 8) & 0xFF;
  for (let i = 0; i < dictLen; i++) {
    p.set(dict[i], 21 + i * 16);
  }
  return p;
}

export function buildInjectPayload(devAddr, fctrl, fcnt, fport, confirmed,
                                   nwkSKey, appSKey, appPayload) {
  const plen = appPayload ? appPayload.length : 0;
  const p = new Uint8Array(41 + 2 + plen);
  p[0] = devAddr & 0xFF;
  p[1] = (devAddr >> 8) & 0xFF;
  p[2] = (devAddr >> 16) & 0xFF;
  p[3] = (devAddr >> 24) & 0xFF;
  p[4] = fctrl & 0xFF;
  p[5] = fcnt & 0xFF;
  p[6] = (fcnt >> 8) & 0xFF;
  p[7] = fport & 0xFF;
  p[8] = confirmed ? 1 : 0;
  p.set(nwkSKey, 9);
  p.set(appSKey, 25);
  p[41] = plen & 0xFF;
  p[42] = (plen >> 8) & 0xFF;
  if (appPayload) p.set(appPayload, 43);
  return p;
}

export function buildScanPayload(dwellMs) {
  const p = new Uint8Array(2);
  p[0] = dwellMs & 0xFF;
  p[1] = (dwellMs >> 8) & 0xFF;
  return p;
}

export function buildFuzzPayload(mode, count, delayMs) {
  const p = new Uint8Array(5);
  p[0] = mode & 0xFF;
  p[1] = count & 0xFF;
  p[2] = (count >> 8) & 0xFF;
  p[3] = delayMs & 0xFF;
  p[4] = (delayMs >> 8) & 0xFF;
  return p;
}

/* ---- Parse a decoded LoRaWAN frame from CMD_FRAME_RECV ---- */
export function parseFrame(payload) {
  if (payload.length < 16) return null;
  const mhdr = payload[0];
  const mtype = (mhdr >> 5) & 0x07;
  const devAddr = payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24);
  const fctrl = payload[5];
  const fcnt = payload[6] | (payload[7] << 8);
  const fport = payload[8];
  const payloadLen = payload[9] | (payload[10] << 8);
  const frmPayload = payload.slice(11, 11 + payloadLen);
  const metaOff = 11 + payloadLen;
  const rssi = (payload[metaOff] | (payload[metaOff+1] << 8)) << 16 >> 16; /* sign-extend */
  const snr = (payload[metaOff+2] << 24) >> 24;
  const freqHz = payload[metaOff+3] | (payload[metaOff+4] << 8) |
                (payload[metaOff+5] << 16) | (payload[metaOff+6] << 24);
  const sf = payload[metaOff+7];
  const tsMs = payload[metaOff+8] | (payload[metaOff+9] << 8) |
               (payload[metaOff+10] << 16) | (payload[metaOff+11] << 24);
  const validMic = payload[metaOff+12];

  const mtypeNames = {
    0: 'JoinReq', 1: 'JoinAccept', 2: 'UnconfUp', 3: 'UnconfDown',
    4: 'ConfUp', 5: 'ConfDown', 6: 'RFU', 7: 'Proprietary',
  };

  return {
    mhdr, mtype, mtypeName: mtypeNames[mtype] || '?',
    devAddr: '0x' + devAddr.toString(16).padStart(8, '0'),
    fctrl: '0x' + fctrl.toString(16).padStart(2, '0'),
    fcnt, fport,
    payloadHex: Array.from(frmPayload).map(b => b.toString(16).padStart(2, '0')).join(''),
    payloadLen,
    rssi, snr, freqHz, sf, tsMs, validMic: validMic ? true : false,
  };
}

export function parseJoinRequest(payload) {
  if (payload.length < 23) return null;
  const joinEui = Array.from(payload.slice(1, 9))
    .map(b => b.toString(16).padStart(2, '0')).join(':');
  const devEui = Array.from(payload.slice(9, 17))
    .map(b => b.toString(16).padStart(2, '0')).join(':');
  const devNonce = payload[17] | (payload[18] << 8);
  const mic = Array.from(payload.slice(19, 23))
    .map(b => b.toString(16).padStart(2, '0')).join('');
  const metaOff = 23;
  if (payload.length >= metaOff + 9) {
    const rssi = (payload[metaOff] | (payload[metaOff+1] << 8)) << 16 >> 16;
    const snr = (payload[metaOff+2] << 24) >> 24;
    const freqHz = payload[metaOff+3] | (payload[metaOff+4] << 8) |
                  (payload[metaOff+5] << 16) | (payload[metaOff+6] << 24);
    const sf = payload[metaOff+7];
    const tsMs = payload[metaOff+8] | (payload[metaOff+9] << 8) |
                 (payload[metaOff+10] << 16) | (payload[metaOff+11] << 24);
    return { joinEui, devEui, devNonce, mic, rssi, snr, freqHz, sf, tsMs };
  }
  return { joinEui, devEui, devNonce, mic };
}

export function parseStatus(payload) {
  if (payload.length < 24) return null;
  const mode = payload[0];
  const region = payload[1];
  const txPower = (payload[2] << 24) >> 24; /* signed */
  const sniffFreq = payload[3] | (payload[4] << 8) | (payload[5] << 16) | (payload[6] << 24);
  const sniffSf = payload[7];
  const sniffBw = payload[8];
  const sniffActive = payload[9];
  const gwemuActive = payload[10];
  const batMv = payload[11] | (payload[12] << 8);
  const sdFreeKb = payload[13] | (payload[14] << 8) | (payload[15] << 16) | (payload[16] << 24);
  const frameCount = payload[17] | (payload[18] << 8) | (payload[19] << 16) | (payload[20] << 24);
  const uptimeMs = payload[21] | (payload[22] << 8) | (payload[23] << 16) | (payload[24] << 24);
  return { mode, region, txPower, sniffFreq, sniffSf, sniffBw,
           sniffActive: !!sniffActive, gwemuActive: !!gwemuActive,
           batMv, sdFreeKb, frameCount, uptimeMs };
}

export function parseScanResult(payload) {
  if (payload.length < 1) return [];
  const count = payload[0];
  const results = [];
  let off = 1;
  for (let i = 0; i < count && off + 11 <= payload.length; i++) {
    const freqHz = payload[off] | (payload[off+1] << 8) | (payload[off+2] << 16) | (payload[off+3] << 24);
    const rssi = (payload[off+4] | (payload[off+5] << 8)) << 16 >> 16;
    const activity = payload[off+6];
    const hits = payload[off+7] | (payload[off+8] << 8) | (payload[off+9] << 16) | (payload[off+10] << 24);
    results.push({ freqHz, rssi, activity: !!activity, hits });
    off += 11;
  }
  return results;
}

export function parseKeySearchResult(payload) {
  if (payload.length < 5) return null;
  const found = payload[0] === 1;
  const trials = payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24);
  let key = null, nwkSKey = null, appSKey = null;
  if (found && payload.length >= 21) {
    key = Array.from(payload.slice(5, 21)).map(b => b.toString(16).padStart(2, '0')).join('');
    if (payload.length >= 37) {
      nwkSKey = Array.from(payload.slice(21, 37)).map(b => b.toString(16).padStart(2, '0')).join('');
    }
    if (payload.length >= 53) {
      appSKey = Array.from(payload.slice(37, 53)).map(b => b.toString(16).padStart(2, '0')).join('');
    }
  }
  return { found, trials, key, nwkSKey, appSKey };
}

/* ---- Hex helpers ---- */
export function hexStr(arr) {
  return Array.from(arr).map(b => (b & 0xFF).toString(16).padStart(2, '0')).join('');
}
export function hexToBytes(hex) {
  const arr = [];
  for (let i = 0; i < hex.length; i += 2) {
    arr.push(parseInt(hex.substr(i, 2), 16));
  }
  return new Uint8Array(arr);
}