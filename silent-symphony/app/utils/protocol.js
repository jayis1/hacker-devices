/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * BLE Wire Protocol — Encode / Decode / CRC-16
 *
 * Matches the firmware's BLE packet format exactly.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

'use strict';

// =========================================================================
// Constants
// =========================================================================

export const BLE_START_BYTE = 0xAA;
export const BLE_HEADER_SIZE = 5;  // START(1) + LENGTH(2) + CMD(2)
export const BLE_CRC_SIZE = 2;
export const BLE_MAX_PAYLOAD = 128;

// Command IDs — matches firmware ble_bridge.h
export const CMD = {
  GET_STATUS: 0x01,
  STATUS_RESP: 0x02,
  SET_MODE: 0x03,
  SET_FSK_PARAMS: 0x04,
  SET_OOK_PARAMS: 0x05,
  TX_MESSAGE: 0x06,
  TX_INTERLEAVED: 0x07,
  RX_START: 0x10,
  RX_STOP: 0x11,
  RX_DATA: 0x12,
  RX_SPECTRUM: 0x13,
  RX_SIGNAL_QUALITY: 0x14,
  BEACON_SET: 0x20,
  BEACON_DATA: 0x21,
  SET_POWER: 0x30,
  SET_GAIN: 0x31,
  STORE_CAPTURE: 0x40,
  RETRIEVE_CAPTURE: 0x41,
  ERASE_STORAGE: 0x42,
  UPDATE_FIRMWARE: 0x50,
  RESET: 0xFF,
};

// Device modes — matches board.h
export const MODE = {
  IDLE: 0,
  TX_FSK: 1,
  TX_OOK: 2,
  TX_WHISPER: 3,
  RX_FSK: 4,
  RX_OOK: 5,
  RX_WHISPER: 6,
  BEACON: 7,
  SCAN: 8,
  SLEEP: 9,
  DFU: 10,
};

export const MODE_NAMES = {
  0: 'Idle',
  1: 'Tx FSK',
  2: 'Tx OOK',
  3: 'Tx Whisper',
  4: 'Rx FSK',
  5: 'Rx OOK',
  6: 'Rx Whisper',
  7: 'Beacon',
  8: 'Scan',
  9: 'Sleep',
  10: 'DFU',
};

export const TX_POWER = {
  WHISPER: 0,
  LOW: 1,
  MED: 2,
  HIGH: 3,
};

export const TX_POWER_NAMES = {
  0: 'Whisper',
  1: 'Low',
  2: 'Medium',
  3: 'High',
};

export const RX_GAIN = {
  LOW: 0,
  MED: 1,
  HIGH: 2,
  AUTO: 3,
};

export const RX_GAIN_NAMES = {
  0: '×1 (0dB)',
  1: '×10 (20dB)',
  2: '×50 (34dB)',
  3: 'Auto',
};

// =========================================================================
// CRC-16-IBM
// =========================================================================

const CRC16_POLY = 0x8005;

function crc16(data: Uint8Array | number[]): number {
  let crc = 0x0000;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ CRC16_POLY;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// =========================================================================
// Packet Encoding
// =========================================================================

/**
 * Encode a command into a BLE packet buffer.
 *
 * @param cmd     Command ID (from CMD enum)
 * @param payload Payload bytes (may be empty)
 * @returns       Uint8Array ready for transmission
 */
export function encodePacket(cmd: number, payload: Uint8Array | number[]): Uint8Array {
  const payloadArr = payload instanceof Uint8Array ? payload : new Uint8Array(payload);
  const pktLen = 2 + payloadArr.length + 2; // cmd(2) + payload + crc(2)
  const buf = new Uint8Array(BLE_HEADER_SIZE + payloadArr.length + BLE_CRC_SIZE);
  let idx = 0;

  buf[idx++] = BLE_START_BYTE;
  buf[idx++] = pktLen & 0xFF;
  buf[idx++] = (pktLen >> 8) & 0xFF;
  buf[idx++] = cmd & 0xFF;
  buf[idx++] = (cmd >> 8) & 0xFF;

  if (payloadArr.length > 0) {
    buf.set(payloadArr, idx);
    idx += payloadArr.length;
  }

  const crcVal = crc16(Array.from(buf.subarray(0, idx)));
  buf[idx++] = crcVal & 0xFF;
  buf[idx++] = (crcVal >> 8) & 0xFF;

  return buf.subarray(0, idx);
}

// =========================================================================
// Packet Decoding
// =========================================================================

export interface DecodedPacket {
  cmd: number;
  payload: Uint8Array;
  valid: boolean;
}

/**
 * Parse state for streaming BLE data.
 */
export class PacketParser {
  private state: 'start' | 'length' | 'cmd' | 'payload' | 'crc' = 'start';
  private buf: number[] = [];
  private expectedLength: number = 0;
  private expectedCmd: number = 0;
  private expectedPayloadLen: number = 0;

  /**
   * Feed a byte into the parser. Returns a decoded packet when complete.
   *
   * @param byte  Raw byte from BLE characteristic
   * @returns     DecodedPacket if a complete valid packet was received, null otherwise
   */
  feed(byte: number): DecodedPacket | null {
    switch (this.state) {
      case 'start':
        if (byte === BLE_START_BYTE) {
          this.buf = [byte];
          this.state = 'length';
        }
        break;

      case 'length':
        this.buf.push(byte);
        if (this.buf.length >= 3) {
          this.expectedLength = this.buf[1] | (this.buf[2] << 8);
          if (this.expectedLength > 255) {
            this.reset();
          } else {
            this.state = 'cmd';
          }
        }
        break;

      case 'cmd':
        this.buf.push(byte);
        if (this.buf.length >= 5) {
          this.expectedCmd = this.buf[3] | (this.buf[4] << 8);
          this.expectedPayloadLen = this.expectedLength - 4; // cmd(2) + crc(2)
          if (this.expectedPayloadLen > BLE_MAX_PAYLOAD) {
            this.reset();
          } else if (this.expectedPayloadLen === 0) {
            this.state = 'crc';
          } else {
            this.state = 'payload';
          }
        }
        break;

      case 'payload':
        this.buf.push(byte);
        if (this.buf.length >= 5 + this.expectedPayloadLen) {
          this.state = 'crc';
        }
        break;

      case 'crc':
        this.buf.push(byte);
        if (this.buf.length >= 5 + this.expectedPayloadLen + 2) {
          // Complete packet — verify CRC
          const dataPart = this.buf.slice(0, -2);
          const receivedCrc = this.buf[this.buf.length - 2] | (this.buf[this.buf.length - 1] << 8);
          const computedCrc = crc16(dataPart);

          const valid = computedCrc === receivedCrc;
          const payload = valid
            ? new Uint8Array(this.buf.slice(5, 5 + this.expectedPayloadLen))
            : new Uint8Array(0);

          const result: DecodedPacket = {
            cmd: this.expectedCmd,
            payload,
            valid,
          };

          this.reset();
          return result;
        }
        break;
    }

    return null;
  }

  private reset(): void {
    this.state = 'start';
    this.buf = [];
    this.expectedLength = 0;
    this.expectedCmd = 0;
    this.expectedPayloadLen = 0;
  }
}

// =========================================================================
// Status Response Parser
// =========================================================================

export interface DeviceStatus {
  batteryPct: number;
  batteryMv: number;
  mode: number;
  signalQuality: number;
  storageUsed: number;
  storageTotal: number;
  txPower: number;
  rxGain: number;
  uptimeS: number;
  fwVersion: string;
}

/**
 * Parse a STATUS_RESP payload.
 *
 * Payload format (20 bytes):
 *   [0]    batteryPct (uint8)
 *   [1-2]  batteryMv (uint16 LE)
 *   [3]    mode (uint8)
 *   [4]    signalQuality (uint8)
 *   [5-8]  storageUsed (uint32 LE)
 *   [9-12] storageTotal (uint32 LE)
 *   [13]   txPower (uint8)
 *   [14]   rxGain (uint8)
 *   [15-18] uptimeS (uint32 LE)
 *   [19]   fwVersionMajor (uint8)
 *   [20]   fwVersionMinor (uint8)
 *   [21]   fwVersionPatch (uint8)
 */
export function parseStatus(payload: Uint8Array): DeviceStatus {
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  let offset = 0;

  const status: DeviceStatus = {
    batteryPct: view.getUint8(offset++),
    batteryMv: view.getUint16(offset, true), offset += 2,
    mode: view.getUint8(offset++),
    signalQuality: view.getUint8(offset++),
    storageUsed: view.getUint32(offset, true), offset += 4,
    storageTotal: view.getUint32(offset, true), offset += 4,
    txPower: view.getUint8(offset++),
    rxGain: view.getUint8(offset++),
    uptimeS: view.getUint32(offset, true), offset += 4,
    fwVersion: `${view.getUint8(offset++)}.${view.getUint8(offset++)}.${view.getUint8(offset++)}`,
  };

  return status;
}

// =========================================================================
// Convenience Command Builders
// =========================================================================

export function buildSetMode(mode: number): Uint8Array {
  return encodePacket(CMD.SET_MODE, [mode]);
}

export function buildSetFskParams(
  markFreq: number,
  spaceFreq: number,
  baud: number,
  amplitude: number,  // 0.0–1.0
): Uint8Array {
  const payload = new Uint8Array(8);
  const view = new DataView(payload.buffer);
  view.setUint16(0, markFreq, true);
  view.setUint16(2, spaceFreq, true);
  view.setUint16(4, baud, true);
  view.setUint8(6, Math.round(amplitude * 100));
  view.setUint8(7, 0); // reserved
  return encodePacket(CMD.SET_FSK_PARAMS, payload);
}

export function buildSetOokParams(
  carrierFreq: number,
  bitDurationUs: number,
  amplitude: number,
): Uint8Array {
  const payload = new Uint8Array(7);
  const view = new DataView(payload.buffer);
  view.setUint16(0, carrierFreq, true);
  view.setUint32(2, bitDurationUs, true);
  view.setUint8(6, Math.round(amplitude * 100));
  return encodePacket(CMD.SET_OOK_PARAMS, payload);
}

export function buildTxMessage(message: string | Uint8Array): Uint8Array {
  const data = typeof message === 'string'
    ? new TextEncoder().encode(message)
    : message;
  return encodePacket(CMD.TX_MESSAGE, data);
}

export function buildSetPower(level: number): Uint8Array {
  return encodePacket(CMD.SET_POWER, [level]);
}

export function buildSetGain(level: number): Uint8Array {
  return encodePacket(CMD.SET_GAIN, [level]);
}

export function buildGetStatus(): Uint8Array {
  return encodePacket(CMD.GET_STATUS, []);
}

export function buildStoreCapture(modType: number, quality: number): Uint8Array {
  return encodePacket(CMD.STORE_CAPTURE, [modType, quality]);
}

export function buildRetrieveCapture(slot: number): Uint8Array {
  return encodePacket(CMD.RETRIEVE_CAPTURE, [slot]);
}

export function buildEraseStorage(): Uint8Array {
  return encodePacket(CMD.ERASE_STORAGE, []);
}

export function buildReset(): Uint8Array {
  return encodePacket(CMD.RESET, []);
}