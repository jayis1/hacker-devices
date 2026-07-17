// src/protocol.ts — CC-Stiletto USB-CDC line protocol definitions.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
//
// The device exposes a USB-CDC port that speaks newline-terminated JSON.
// Commands are sent as {"cmd":...} objects; events arrive as {"evt":...}.

// ---- PD message types (mirror of firmware pd_stack.h) -------------------
export const PD_CTRL = {
  GOODCRC: 0x01, GOTOMIN: 0x02, ACCEPT: 0x03, REJECT: 0x04, PING: 0x05,
  PS_RDY: 0x06, GET_SOURCE_CAP: 0x07, GET_SINK_CAP: 0x08, DR_SWAP: 0x09,
  PR_SWAP: 0x0a, VCONN_SWAP: 0x0b, WAIT: 0x0c, SOFT_RESET: 0x0d,
  NOT_SUPPORTED: 0x10, GET_SOURCE_CAP_EXT: 0x11, GET_STATUS: 0x12,
  FR_SWAP: 0x13, GET_PPS_STATUS: 0x14, GET_COUNTRY_CODES: 0x15,
} as const;

export const PD_DATA = {
  SOURCE_CAP: 0x00, REQUEST: 0x02, BIST: 0x03, SINK_CAP: 0x04,
  BATTERY_STATUS: 0x05, GET_COUNTRY_INFO: 0x06, ENTER_USB: 0x09,
  VENDOR_DEFINED: 0x0f,
} as const;

export const CTRL_NAMES: Record<number, string> = {
  0x00: 'Reserved', 0x01: 'GoodCRC', 0x02: 'GotoMin', 0x03: 'Accept',
  0x04: 'Reject', 0x05: 'Ping', 0x06: 'PS_RDY', 0x07: 'Get_Source_Cap',
  0x08: 'Get_Sink_Cap', 0x09: 'DR_Swap', 0x0a: 'PR_Swap',
  0x0b: 'VCONN_Swap', 0x0c: 'Wait', 0x0d: 'Soft_Reset', 0x0e: 'DR_Swap3',
  0x10: 'Not_Supported', 0x11: 'Get_Source_Cap_Ext', 0x12: 'Get_Status',
  0x13: 'FR_Swap', 0x14: 'Get_PPS_Status', 0x15: 'Get_Country_Codes',
};

export const DATA_NAMES: Record<number, string> = {
  0x00: 'Source_Capabilities', 0x02: 'Request', 0x03: 'BIST',
  0x04: 'Sink_Capabilities', 0x05: 'Battery_Status',
  0x06: 'Get_Country_Info', 0x09: 'Enter_USB',
  0x0f: 'Vendor_Defined',
};

export const SOP_NAMES = ['SOP', "SOP'", "SOP''"];

export interface PdMsg {
  sop: number;          // 0=SOP, 1=SOP', 2=SOP''
  header: number;       // raw 16-bit header
  msg_id: number;
  num_obj: number;
  obj: number[];
  direction?: 'src->snk' | 'snk->src';
  timestamp: number;
}

// ---- Commands we can send ------------------------------------------------
export type Cmd =
  | { cmd: 'mode'; mode: ModeName }
  | { cmd: 'spoof'; mv: number; ma: number }
  | { cmd: 'glitch'; hi: number; lo: number; hi_us: number; lo_us: number; rep: number }
  | { cmd: 'hijack'; dr: 0 | 1; pr: 0 | 1; frs: 0 | 1; ms: number }
  | { cmd: 'inject'; type: number; nobj: number; obj?: number[] }
  | { cmd: 'arm' }
  | { cmd: 'disarm' }
  | { cmd: 'telemetry' }
  | { cmd: 'help' };

export type ModeName =
  | 'sniff' | 'inject' | 'spoof' | 'glitch'
  | 'role-hijack' | 'vconn-hijack' | 'dead-battery' | 'fuzz';

export const MODES: { id: ModeName; label: string; color: string }[] = [
  { id: 'sniff',         label: 'Sniff',         color: '#2a9d8f' },
  { id: 'inject',        label: 'Inject',        color: '#e63946' },
  { id: 'spoof',         label: 'Spoof Voltage', color: '#9b5de5' },
  { id: 'glitch',        label: 'Glitch',        color: '#f4a261' },
  { id: 'role-hijack',   label: 'Role Hijack',   color: '#4cc9f0' },
  { id: 'vconn-hijack',  label: 'VCONN Hijack',  color: '#ffd60a' },
  { id: 'dead-battery',  label: 'Dead Battery',  color: '#3a86ff' },
  { id: 'fuzz',          label: 'Fuzz',          color: '#ff70a6' },
];

// ---- Telemetry snapshot --------------------------------------------------
export interface Telemetry {
  src_mv: number; src_ma: number;
  snk_mv: number; snk_ma: number;
  temp_c: number;
  efuse_fault: boolean;
}

// ---- JSON line codec -----------------------------------------------------
export function encodeCmd(c: Cmd): string {
  return JSON.stringify(c) + '\n';
}

export function parseLine(line: string): PdMsg | Telemetry | null {
  try {
    const o = JSON.parse(line);
    if (typeof o !== 'object' || o === null) return null;
    if (typeof o.evt === 'string' && o.evt.indexOf('pd') >= 0) {
      return {
        sop: o.sop ?? 0,
        header: o.hdr ?? 0,
        msg_id: o.id ?? 0,
        num_obj: o.nobj ?? 0,
        obj: o.obj ?? [],
        direction: o.tag,
        timestamp: Date.now(),
      } as PdMsg;
    }
    if (typeof o.telemetry !== 'undefined') {
      // device emits: telemetry src_v=X src_i=Y snk_v=Z snk_i=W temp=TC
      const m = line.match(/src_v=(\d+)mV src_i=(-?\d+)mA snk_v=(\d+)mV snk_i=(-?\d+)mA temp=(\d+)C/);
      if (m) {
        return {
          src_mv: +m[1], src_ma: +m[2], snk_mv: +m[3], snk_ma: +m[4],
          temp_c: +m[5], efuse_fault: false,
        } as Telemetry;
      }
    }
  } catch { /* ignore malformed lines */ }
  return null;
}

// Parse a Fixed Source PDO into {mv, ma}
export function parseFixedPdo(pdo: number): { mv: number; ma: number } {
  return { mv: ((pdo >> 10) & 0x3ff) * 50, ma: (pdo & 0x3ff) * 10 };
}

// Build a Fixed PDO 32-bit value
export function buildFixedPdo(mv: number, ma: number): number {
  return (((mv / 50) & 0x3ff) << 10) | ((ma / 10) & 0x3ff);
}

// Build a Request Data Object (RDO) for a fixed PDO index
export function buildRdo(idx: number, ma: number, usbComms = true): number {
  let r = ((idx + 1) & 0x7) << 28;
  r |= (ma / 10) & 0x3ff;            // operating current
  r |= ((ma / 10) & 0x3ff) << 10;    // max operating current
  if (usbComms) r |= 1 << 26;
  r |= 1 << 24;                       // no USB suspend
  return r >>> 0;
}