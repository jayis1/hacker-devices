/**
 * protocol.js — Binary frame encoder/decoder for BLE C2 protocol
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Frame format:
 *   [START:0xAA][OP:1][LEN:1][PAYLOAD:LEN][CRC:1]
 *   CRC = XOR of all bytes from OP..PAYLOAD.
 *
 * This must match the firmware's ble_c2.c parser exactly.
 */

export const OPCODES = {
  // Commands (app → wand)
  CMD_SET_MODE:        0x01,
  CMD_SET_TX_POWER:    0x02,
  CMD_SET_PULSE:       0x03,
  CMD_SET_THRESH:      0x04,
  CMD_SET_QUIET:       0x05,
  CMD_QUERY_STATUS:    0x06,
  CMD_EXPORT:          0x07,

  // Notifications (wand → app)
  NOTIF_STATUS:        0x81,
  NOTIF_HIT:           0x82,
  NOTIF_EXPORT_DONE:   0x83,
};

export const MODES = {
  IDLE:         0,
  QUIET_RX:     1,
  SWEEP_CW:     2,
  SWEEP_PULSED: 3,
  CALIBRATE:    4,
  FAULT:        5,
};

export const CLASSIFY = {
  NONE:      0,
  SEMI:      1,
  METAL:     2,
  AMBIGUOUS: 3,
};

/**
 * Encode a command frame.
 * @param {number} op - opcode from OPCODES
 * @param {number[]} payload - byte array
 * @returns {number[]} complete frame including start, CRC
 */
export function encodeFrame(op, payload = []) {
  const len = payload.length;
  let crc = op ^ len;
  for (let i = 0; i < len; i++) crc ^= payload[i];
  return [0xAA, op, len, ...payload, crc & 0xFF];
}

/**
 * Decode complete frames from a byte buffer.
 * Consumes used bytes from the front of the array (mutates in place).
 * @param {number[]} buf - mutable RX buffer
 * @returns {Array<{op: number, payload: number[]}>} decoded frames
 */
export function decodeFrame(buf) {
  const frames = [];
  while (buf.length >= 5) {
    // Find start byte
    if (buf[0] !== 0xAA) {
      buf.shift();
      continue;
    }
    const op = buf[1];
    const len = buf[2];
    if (buf.length < 4 + len) break;  // incomplete

    let crc = op ^ len;
    for (let i = 0; i < len; i++) crc ^= buf[3 + i];
    const recvCrc = buf[3 + len];

    if (crc === recvCrc) {
      frames.push({
        op,
        payload: buf.slice(3, 3 + len),
      });
    }
    // Remove consumed bytes whether or not CRC matched
    buf.splice(0, 4 + len);
  }
  return frames;
}