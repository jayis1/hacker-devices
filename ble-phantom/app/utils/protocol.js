/**
 * protocol.js — BLE Phantom USB communication protocol
 *
 * Defines the binary protocol between the companion app
 * and the BLE Phantom hardware over USB CDC serial.
 *
 * Frame format:
 *   [0xAA] [0x55] [CMD] [LEN_H] [LEN_L] [PAYLOAD...] [CRC_H] [CRC_L] [0x55] [0xAA]
 *
 * Host → Device commands:
 *   0xA0 SET_MODE      - Set operating mode
 *   0xA1 SET_CONFIG    - Configure radio parameters
 *   0xA2 START          - Start operation (scan/adv/connect)
 *   0xA3 STOP           - Stop operation
 *   0xA4 REPLAY         - Replay captured packet
 *   0xA5 GET_STATUS     - Request device status
 *   0xA6 GET_PACKET     - Request next captured packet
 *   0xA7 SET_ADV_DATA   - Set advertising data
 *   0xA8 SET_CHAN        - Set BLE channel
 *   0xA9 SET_TX_PWR     - Set TX power
 *   0xB0 MODE_NOTIFY    - Mode change notification
 *   0xFF RESET           - Reset device
 *
 * Device → Host responses:
 *   0x01 Radio A data   - Packet captured by Radio A
 *   0x02 Radio B data    - Packet captured by Radio B
 *   0xA0 SET_MODE_ACK    - Mode change acknowledged
 *   0xA1 SET_CONFIG_ACK  - Configuration acknowledged
 *   0xA5 STATUS_RESP     - Status response
 */

// ============================================================
// Protocol constants
// ============================================================

export const SYNC_WORD      = 0xAA55;
export const TRAILER_WORD   = 0x55AA;

// Host → Device commands
export const CMD_SET_MODE     = 0xA0;
export const CMD_SET_CONFIG   = 0xA1;
export const CMD_START        = 0xA2;
export const CMD_STOP         = 0xA3;
export const CMD_REPLAY       = 0xA4;
export const CMD_GET_STATUS   = 0xA5;
export const CMD_GET_PACKET   = 0xA6;
export const CMD_SET_ADV_DATA = 0xA7;
export const CMD_SET_CHAN     = 0xA8;
export const CMD_SET_TX_PWR   = 0xA9;
export const CMD_MODE_NOTIFY  = 0xB0;
export const CMD_RESET        = 0xFF;

// Device → Host markers
export const RADIO_A_MARKER = 0x01;
export const RADIO_B_MARKER = 0x02;

// Operating modes
export const MODE_IDLE       = 0;
export const MODE_SNIFF      = 1;
export const MODE_SCAN       = 2;
export const MODE_ADVERTISE  = 3;
export const MODE_CONNECT    = 4;
export const MODE_MITM       = 5;
export const MODE_REPLAY     = 6;

// Radio status flags
export const STATUS_INITIALIZED = 0x01;
export const STATUS_SCANNING    = 0x02;
export const STATUS_ADVERTISING = 0x04;
export const STATUS_CONNECTED   = 0x08;
export const STATUS_RX_PENDING  = 0x10;
export const STATUS_TX_COMPLETE = 0x20;
export const STATUS_ERROR       = 0x80;

// BLE PHY modes
export const PHY_1M    = 1;
export const PHY_2M    = 2;
export const PHY_CODED = 3;

// BLE advertising channels
export const CHAN_ADV_37 = 37;
export const CHAN_ADV_38 = 38;
export const CHAN_ADV_39 = 39;

// ============================================================
// CRC16-CCITT
// ============================================================

export function crc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }
  return crc;
}

// ============================================================
// Frame builder (Host → Device)
// ============================================================

export function buildFrame(cmd, payload = []) {
  const len = payload.length;
  const header = [
    (SYNC_WORD >> 8) & 0xFF,
    SYNC_WORD & 0xFF,
    cmd,
    (len >> 8) & 0xFF,
    len & 0xFF,
  ];

  const frame = [...header, ...payload];
  const checksum = crc16(frame);
  frame.push((checksum >> 8) & 0xFF);
  frame.push(checksum & 0xFF);
  frame.push((TRAILER_WORD >> 8) & 0xFF);
  frame.push(TRAILER_WORD & 0xFF);

  return Uint8Array.from(frame);
}

// ============================================================
// Frame parser (Device → Host)
// ============================================================

export function parseFrame(buffer) {
  if (buffer.length < 9) return null; // Minimum: sync(2) + cmd(1) + len(2) + crc(2) + trailer(2)

  // Find sync word
  const syncIdx = findSync(buffer);
  if (syncIdx === -1) return null;

  const aligned = buffer.slice(syncIdx);
  if (aligned.length < 5) return null;

  const cmd = aligned[2];
  const len = (aligned[3] << 8) | aligned[4];
  const totalLen = 5 + len + 4; // header + payload + crc + trailer

  if (aligned.length < totalLen) return null; // Incomplete

  const payload = aligned.slice(5, 5 + len);

  // Verify CRC
  const expectedCrc = crc16(aligned.slice(0, 5 + len));
  const actualCrc = (aligned[5 + len] << 8) | aligned[5 + len + 1];
  if (expectedCrc !== actualCrc) return null; // CRC error

  // Verify trailer
  const trailerOffset = 5 + len + 2;
  const actualTrailer = (aligned[trailerOffset] << 8) | aligned[trailerOffset + 1];
  if (actualTrailer !== TRAILER_WORD) return null;

  return {
    cmd,
    payload: Uint8Array.from(payload),
    consumed: syncIdx + totalLen,
  };
}

function findSync(buffer) {
  for (let i = 0; i < buffer.length - 1; i++) {
    if (buffer[i] === 0xAA && buffer[i + 1] === 0x55) {
      return i;
    }
  }
  return -1;
}

// ============================================================
// Command builders
// ============================================================

export function cmdSetMode(mode) {
  return buildFrame(CMD_SET_MODE, [mode]);
}

export function cmdSetConfig(radio, channel, txPower, advInterval, scanWindow, scanInterval, phy) {
  return buildFrame(CMD_SET_CONFIG, [
    radio,
    channel,
    txPower & 0xFF,
    (advInterval >> 8) & 0xFF, advInterval & 0xFF,
    (scanWindow >> 8) & 0xFF, scanWindow & 0xFF,
    (scanInterval >> 8) & 0xFF, scanInterval & 0xFF,
    phy,
  ]);
}

export function cmdStart(radio) {
  return buildFrame(CMD_START, [radio]);
}

export function cmdStop(radio) {
  return buildFrame(CMD_STOP, [radio]);
}

export function cmdReplay(radio, data) {
  return buildFrame(CMD_REPLAY, [radio, ...data]);
}

export function cmdGetStatus() {
  return buildFrame(CMD_GET_STATUS);
}

export function cmdSetChannel(radio, channel) {
  return buildFrame(CMD_SET_CHAN, [radio, channel]);
}

export function cmdSetTxPower(radio, power) {
  return buildFrame(CMD_SET_TX_PWR, [radio, power & 0xFF]);
}

export function cmdReset() {
  return buildFrame(CMD_RESET);
}

// ============================================================
// Response parsers
// ============================================================

export function parseStatusResponse(payload) {
  if (payload.length < 4) return null;
  return {
    mode: payload[0],
    radioA: payload[1],
    radioB: payload[2],
    timestamp: (payload[3] << 0) | (payload[4] << 8) | (payload[5] << 16) | (payload[6] << 24),
  };
}

export function parseRadioPacket(payload) {
  if (payload.length < 4) return null;
  const len = (payload[2] << 8) | payload[3];
  return {
    radio: payload[0],
    cmd: payload[1],
    length: len,
    data: payload.slice(4, 4 + len),
  };
}

// ============================================================
// Utility functions
// ============================================================

export function formatMAC(bytes) {
  return Array.from(bytes)
    .map(b => b.toString(16).padStart(2, '0').toUpperCase())
    .join(':');
}

export function formatRSSI(raw) {
  return raw > 127 ? raw - 256 : raw;
}

export function channelName(ch) {
  if (ch === 37) return 'ADV37';
  if (ch === 38) return 'ADV38';
  if (ch === 39) return 'ADV39';
  if (ch >= 0 && ch <= 36) return `D${ch}`;
  return `CH${ch}`;
}

export function advTypeName(type) {
  const names = {
    0x00: 'ADV_IND',
    0x01: 'ADV_DIRECT_IND',
    0x02: 'ADV_NONCONN_IND',
    0x03: 'SCAN_REQ',
    0x04: 'SCAN_RSP',
    0x05: 'ADV_EXT_IND',
  };
  return names[type] || `0x${type.toString(16).padStart(2, '0')}`;
}

export function bytesToHex(bytes) {
  return Array.from(bytes).map(b => b.toString(16).padStart(2, '0')).join(' ');
}