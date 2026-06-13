/**
 * WFP-100 Wire Protocol — Command/Response format
 *
 * The WFP-100 communicates with the companion app over USB CDC-ACM
 * (serial) or CDC-ECM (network). This module defines the binary
 * protocol for device control and packet data streaming.
 *
 * Protocol format:
 *   [SYNC: 0xAA55] [CMD: u8] [LEN: u16 LE] [PAYLOAD: LEN bytes] [CRC16: u16 LE]
 *
 * SPDX-License-Identifier: MIT
 */

// Sync bytes
export const PROTO_SYNC = 0xAA55;

// Command IDs (device ← app)
export const CMD_CONNECT          = 0x01;
export const CMD_DISCONNECT       = 0x02;
export const CMD_START_CAPTURE    = 0x03;
export const CMD_STOP_CAPTURE     = 0x04;
export const CMD_SET_CHANNEL      = 0x05;
export const CMD_SET_BAND         = 0x06;
export const CMD_INJECT_FRAME     = 0x07;
export const CMD_GET_STATUS       = 0x08;
export const CMD_SET_FILTER       = 0x09;
export const CMD_EXPORT_PCAP     = 0x0A;
export const CMD_SET_MODE         = 0x0B;  // idle/monitor/inject/evil-twin
export const CMD_WIFI_KILL        = 0x0C;

// Response IDs (device → app)
export const RESP_STATUS          = 0x81;
export const RESP_PACKET          = 0x82;
export const RESP_CAPTURE_STARTED = 0x83;
export const RESP_CAPTURE_STOPPED = 0x84;
export const RESP_ERROR           = 0x85;
export const RESP_CHANNEL_CHANGED = 0x86;
export const RESP_PMKID_FOUND     = 0x87;
export const RESP_HANDSHAKE      = 0x88;

// WiFi bands
export const BAND_2GHZ = 0;
export const BAND_5GHZ = 1;
export const BAND_6GHZ = 2;

// Capture modes
export const MODE_IDLE       = 0;
export const MODE_MONITOR    = 1;
export const MODE_INJECT     = 2;
export const MODE_EVIL_TWIN  = 3;

// Error codes
export const ERR_UNKNOWN       = 0x00;
export const ERR_NOT_CONNECTED = 0x01;
export const ERR_BUSY          = 0x02;
export const ERR_INVALID_CMD   = 0x03;
export const ERR_INVALID_PARAM = 0x04;
export const ERR_HW_FAILURE   = 0x05;

/**
 * Build a protocol frame for sending to the device.
 * @param {number} cmd - Command ID
 * @param {Uint8Array} payload - Command payload
 * @returns {Uint8Array} Complete frame with sync, header, payload, and CRC
 */
export function buildFrame(cmd, payload) {
  const len = payload ? payload.length : 0;
  const frame = new Uint8Array(7 + len + 2);  // sync(2) + cmd(1) + len(2) + payload + crc(2)

  // Sync bytes
  frame[0] = 0xAA;
  frame[1] = 0x55;

  // Command
  frame[2] = cmd;

  // Length (little-endian)
  frame[3] = len & 0xFF;
  frame[4] = (len >> 8) & 0xFF;

  // Payload
  if (payload) {
    frame.set(payload, 5);
  }

  // CRC16-CCITT over cmd + len + payload
  const crcData = frame.slice(2, 5 + len);
  const crc = crc16_ccitt(crcData);
  frame[5 + len] = crc & 0xFF;
  frame[6 + len] = (crc >> 8) & 0xFF;

  return frame;
}

/**
 * Parse a response frame from the device.
 * @param {Uint8Array} data - Raw bytes from serial/network
 * @returns {Object|null} Parsed frame { cmd, len, payload } or null on error
 */
export function parseFrame(data) {
  if (data.length < 7) return null;  // Minimum frame: sync(2) + cmd(1) + len(2) + crc(2)

  // Check sync bytes
  if (data[0] !== 0xAA || data[1] !== 0x55) return null;

  const cmd = data[2];
  const len = data[3] | (data[4] << 8);

  if (data.length < 5 + len + 2) return null;  // Incomplete frame

  const payload = data.slice(5, 5 + len);
  const receivedCrc = data[5 + len] | (data[6 + len] << 8);

  // Verify CRC
  const crcData = data.slice(2, 5 + len);
  const calculatedCrc = crc16_ccitt(crcData);
  if (receivedCrc !== calculatedCrc) return null;

  return { cmd, len, payload };
}

/**
 * CRC16-CCITT calculation.
 * Polynomial: 0x1021, initial value: 0xFFFF
 */
function crc16_ccitt(data) {
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

/**
 * Parse a packet payload from a RESP_PACKET response.
 * Frame data includes a 12-byte radiotap-like header followed by the 802.11 frame.
 */
export function parsePacketPayload(payload) {
  if (payload.length < 12) return null;

  return {
    channel:   payload[0] | (payload[1] << 8),
    frequency: payload[2] | (payload[3] << 8),
    rssi:      payload[4] > 127 ? payload[4] - 256 : payload[4],  // signed
    noise:     payload[5] > 127 ? payload[5] - 256 : payload[5],
    rate:      payload[6] | (payload[7] << 8),   // 500kbps units
    timestamp: payload[8] | (payload[9] << 16) | (payload[10] << 24) | (payload[11] << 32),
    frame:     payload.slice(12),  // Raw 802.11 frame bytes
  };
}

/**
 * Build a channel change command payload.
 */
export function buildSetChannelPayload(channel, band) {
  const payload = new Uint8Array(3);
  payload[0] = channel & 0xFF;
  payload[1] = (channel >> 8) & 0xFF;
  payload[2] = band;
  return payload;
}

/**
 * Parse device status response.
 */
export function parseStatusPayload(payload) {
  if (payload.length < 16) return null;

  return {
    mode:            payload[0],
    channel:         payload[1] | (payload[2] << 8),
    band:            payload[3],
    monitorEnabled:  !!(payload[4] & 0x01),
    capturing:       !!(payload[4] & 0x02),
    injecting:       !!(payload[4] & 0x04),
    packetsCaptured: payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24),
    packetsInjected: payload[9] | (payload[10] << 8) | (payload[11] << 16) | (payload[12] << 24),
    batteryMv:       payload[13] | (payload[14] << 8),
    temperature:     payload[15] > 127 ? payload[15] - 256 : payload[15],
  };
}