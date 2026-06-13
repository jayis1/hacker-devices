/**
 * protocol.js — Wire Protocol Definitions for RF Transceiver Tool
 *
 * Defines the command IDs, packet format, and CRC8 algorithm
 * used for communication between the companion app and the firmware
 * over USB CDC virtual serial port or BLE.
 *
 * Command format:
 *   [0xAA] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
 *
 * All multi-byte values are little-endian.
 */

// Start-of-packet marker
export const SOP = 0xAA;

// Maximum payload length
export const MAX_PAYLOAD = 252;

// Maximum packet length (SOP + CMD + LEN + PAYLOAD + CRC)
export const MAX_PACKET = MAX_PAYLOAD + 4;

// Command IDs
export const CMD = {
  // System
  PING:               0x01,
  GET_STATUS:         0x02,
  GET_VERSION:        0x03,
  SET_MODE:           0x04,
  RESET:              0x05,

  // CC1101 Sub-GHz
  CC1101_INIT:        0x10,
  CC1101_CONFIG:      0x11,
  CC1101_RX_START:    0x12,
  CC1101_RX_STOP:     0x13,
  CC1101_TX:          0x14,
  CC1101_SCAN_START:  0x15,
  CC1101_SCAN_STOP:   0x16,
  CC1101_PACKET:      0x17,
  CC1101_SET_FREQ:    0x18,
  CC1101_SET_POWER:   0x19,
  CC1101_SET_MOD:     0x1A,
  CC1101_SET_DATARATE:0x1B,
  CC1101_GET_RSSI:    0x1C,
  CC1101_SET_SYNC:    0x1D,
  CC1101_SNIFF:       0x1E,

  // nRF24L01+ 2.4 GHz
  NRF24_INIT:         0x20,
  NRF24_CONFIG:       0x21,
  NRF24_RX_START:     0x22,
  NRF24_RX_STOP:      0x23,
  NRF24_TX:           0x24,
  NRF24_SCAN_START:   0x25,
  NRF24_SCAN_STOP:    0x26,
  NRF24_PACKET:       0x27,
  NRF24_SET_CHANNEL:  0x28,
  NRF24_SET_POWER:    0x29,
  NRF24_SET_DATARATE: 0x2A,
  NRF24_SET_ADDR:     0x2B,
  NRF24_SET_PIPE:     0x2C,
  NRF24_GET_RPD:      0x2D,
  NRF24_SNIFF:        0x2E,
  NRF24_JAM:          0x2F,

  // Display
  DISPLAY_TEXT:       0x30,
  DISPLAY_BAR:        0x31,
  DISPLAY_CLEAR:      0x32,
  DISPLAY_CONTRAST:   0x33,

  // Responses
  ACK:                0xE0,
  NACK:               0xE1,
  ERROR:              0xFF,
};

// Device modes (matches firmware app_mode_t)
export const MODE = {
  IDLE:           0,
  SUBGHZ_RX:      1,
  SUBGHZ_TX:      2,
  SUBGHZ_SCAN:    3,
  NRF24_RX:       4,
  NRF24_TX:       5,
  NRF24_SCAN:     6,
  SUBGHZ_SNIFF:   7,
  NRF24_SNIFF:    8,
};

// Modulation types (matches CC1101)
export const MODULATION = {
  '2-FSK':   0x00,
  'GFSK':    0x10,
  'ASK/OOK': 0x30,
  '4-FSK':   0x40,
  'MSK':     0x70,
};

// nRF24L01+ data rates
export const NRF24_DATARATE = {
  '250 kbps': 0,
  '1 Mbps':   1,
  '2 Mbps':   2,
};

// nRF24L01+ TX power levels
export const NRF24_POWER = {
  '-18 dBm': 0,
  '-12 dBm': 1,
  '-6 dBm':  2,
  '0 dBm':   3,
};

// CC1101 TX power levels
export const CC1101_POWER = {
  '-30 dBm': 0x00,
  '-20 dBm': 0x01,
  '-15 dBm': 0x02,
  '-10 dBm': 0x05,
  '0 dBm':   0x0C,
  '+5 dBm':  0x0F,
  '+7 dBm':  0x11,
  '+10 dBm': 0x1D,
  '+12 dBm': 0x50,
};

// CC1101 data rates
export const CC1101_DATARATE = {
  '1.2 kbps':  0,
  '2.4 kbps':  1,
  '4.8 kbps':  2,
  '9.6 kbps':  3,
  '15.6 kbps': 4,
  '38.4 kbps': 5,
  '76.8 kbps': 6,
  '250 kbps':  7,
  '500 kbps':  8,
};

// Common Sub-GHz frequencies (Hz)
export const SUBGHZ_FREQS = {
  '315 MHz':   315000000,
  '390 MHz':   390000000,
  '433.92 MHz': 433920000,
  '434 MHz':   434000000,
  '868 MHz':   868000000,
  '868.35 MHz': 868350000,
  '915 MHz':   915000000,
  '928 MHz':   928000000,
};

// Common nRF24 channels
export const NRF24_CHANNELS = {
  'Channel 2 (2402 MHz)':  2,
  'Channel 76 (2476 MHz)': 76,
  'Channel 80 (2480 MHz)': 80,
  'Channel 125 (2525 MHz)': 125,
};

/**
 * CRC-8-CCITT calculation
 * Polynomial: 0x07 (x^8 + x^2 + x + 1)
 * Initial value: 0x00
 */
export function crc8(data) {
  let crc = 0x00;
  for (const byte of data) {
    crc ^= byte;
    for (let i = 0; i < 8; i++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ 0x07) & 0xFF;
      } else {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  return crc;
}

/**
 * Build a command packet
 * @param cmdId Command ID from CMD enum
 * @param payload Payload bytes (optional)
 * @returns Complete packet as Uint8Array
 */
export function buildPacket(cmdId, payload = []) {
  if (payload.length > MAX_PAYLOAD) {
    throw new Error(`Payload too long: ${payload.length} > ${MAX_PAYLOAD}`);
  }

  const packet = new Uint8Array(3 + payload.length + 1);
  packet[0] = SOP;
  packet[1] = cmdId;
  packet[2] = payload.length;
  for (let i = 0; i < payload.length; i++) {
    packet[3 + i] = payload[i];
  }

  // CRC over CMD_ID + LEN + PAYLOAD
  const crcData = [cmdId, payload.length, ...payload];
  packet[3 + payload.length] = crc8(crcData);

  return packet;
}

/**
 * Parse a received packet
 * @param data Raw bytes from serial port
 * @returns Parsed packet { cmdId, payload, valid } or null if incomplete
 */
export function parsePacket(data) {
  if (data.length < 4) return null;

  if (data[0] !== SOP) return null;

  const cmdId = data[1];
  const len = data[2];

  if (data.length < 3 + len + 1) return null;

  const payload = Array.from(data.slice(3, 3 + len));
  const receivedCrc = data[3 + len];

  const crcData = [cmdId, len, ...payload];
  const computedCrc = crc8(crcData);

  return {
    cmdId,
    payload,
    valid: receivedCrc === computedCrc,
  };
}

/**
 * Encode a frequency value (Hz) to 3-byte little-endian for CC1101
 */
export function encodeFrequency(freqHz) {
  return [
    (freqHz >> 0) & 0xFF,
    (freqHz >> 8) & 0xFF,
    (freqHz >> 16) & 0xFF,
  ];
}

/**
 * Decode RSSI value from CC1101 raw byte to dBm
 */
export function decodeRSSI(rssiRaw) {
  if (rssiRaw >= 128) {
    return Math.round(((rssiRaw - 256) / 2) - 74);
  }
  return Math.round((rssiRaw / 2) - 74);
}

/**
 * Format frequency for display
 */
export function formatFrequency(freqHz) {
  if (freqHz >= 1000000000) {
    return `${(freqHz / 1000000000).toFixed(3)} GHz`;
  }
  return `${(freqHz / 1000000).toFixed(2)} MHz`;
}

/**
 * Mode name mapping
 */
export const MODE_NAMES = {
  [MODE.IDLE]:          'Idle',
  [MODE.SUBGHZ_RX]:     'Sub-GHz RX',
  [MODE.SUBGHZ_TX]:     'Sub-GHz TX',
  [MODE.SUBGHZ_SCAN]:   'Sub-GHz Scan',
  [MODE.NRF24_RX]:      'nRF24 RX',
  [MODE.NRF24_TX]:      'nRF24 TX',
  [MODE.NRF24_SCAN]:    'nRF24 Scan',
  [MODE.SUBGHZ_SNIFF]:  'Sub-GHz Sniff',
  [MODE.NRF24_SNIFF]:   'nRF24 Sniff',
};