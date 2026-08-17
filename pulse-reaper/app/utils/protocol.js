/**
 * protocol.js — fieldbus frame decoding helpers for the app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Decodes the raw frame payload [proto_id, dst_lo, dst_hi, func, len, data...]
 * sent by the device's BLE C2 into a human-readable summary for the
 * Live Capture screen.
 */

// Protocol IDs (must match firmware protocol_detect.h PR_PROTO_*)
export const PROTO = {
  NONE: 0,
  MODBUS_RTU: 1,
  PROFIBUS_DP: 2,
  HART_FSK: 3,
  CAN: 4,
  POTS_DTMF: 5,
};

export function protocolName(id) {
  switch (id) {
    case PROTO.MODBUS_RTU: return 'Modbus RTU';
    case PROTO.PROFIBUS_DP: return 'Profibus DP';
    case PROTO.HART_FSK: return 'HART FSK';
    case PROTO.CAN: return 'CAN/CAN-FD';
    case PROTO.POTS_DTMF: return 'POTS DTMF/CID';
    default: return 'Unknown';
  }
}

// Modbus function code names
const MODBUS_FUNCS = {
  1: 'Read Coils',
  2: 'Read Discrete Inputs',
  3: 'Read Holding Regs',
  4: 'Read Input Regs',
  5: 'Write Single Coil',
  6: 'Write Single Reg',
  7: 'Read Exception Status',
  8: 'Diagnostics',
  11: 'Get Comm Event Counter',
  15: 'Write Multiple Coils',
  16: 'Write Multiple Regs',
  22: 'Mask Write Reg',
  23: 'Read/Write Multiple Regs',
  24: 'Read FIFO Queue',
};

/**
 * Decode a raw frame payload from BLE into a structured object.
 * Author: jayis1
 *
 * @param {Buffer|number[]} raw  — [proto_id, dst_lo, dst_hi, func, len, data...]
 * @returns {{protocol_id, dst_addr, function, length, data, summary}}
 */
export function decodeFrame(raw) {
  const buf = Array.isArray(raw) ? raw : Array.from(raw);
  if (buf.length < 5) {
    return { protocol_id: 0, dst_addr: 0, function: 0, length: 0, data: [], summary: 'invalid' };
  }
  const proto_id = buf[0];
  const dst_addr = buf[1] | (buf[2] << 8);
  const func = buf[3];
  const len = buf[4];
  const data = buf.slice(5, 5 + len);

  let summary = '';
  switch (proto_id) {
    case PROTO.MODBUS_RTU:
      summary = `Modbus ${MODBUS_FUNCS[func] || 'fn=' + func} slave=${dst_addr} bytes=${len}`;
      if (data.length > 0) {
        summary += ' data=' + data.map((b) => b.toString(16).padStart(2, '0')).join(' ');
      }
      break;
    case PROTO.PROFIBUS_DP:
      summary = `Profibus DP DA=${dst_addr} SA=? FC=0x${func.toString(16)} len=${len}`;
      break;
    case PROTO.HART_FSK:
      summary = `HART cmd=${func} addr=${dst_addr} bytes=${len}`;
      break;
    case PROTO.CAN:
      summary = `CAN ID=0x${dst_addr.toString(16)} DLC=${func} data=${data.map((b) => b.toString(16).padStart(2, '0')).join(' ')}`;
      break;
    case PROTO.POTS_DTMF:
      summary = `POTS digit='${String.fromCharCode(data[0] || '?'.charCodeAt(0))}'`;
      break;
    default:
      summary = `Unknown proto=${proto_id} addr=${dst_addr} fn=${func} len=${len}`;
  }

  return { protocol_id: proto_id, dst_addr, function: func, length: len, data, summary };
}