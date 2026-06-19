/**
 * BleContext.tsx — BLE connection & protocol context for PhotonStrike
 * Author: jayis1
 * License: MIT
 *
 * Manages the BLE GATT connection to the PhotonStrike device and exposes
 * a React context with connection state, scan status, DFA results, and
 * command methods. Uses react-native-ble-plx.
 */

import React, { createContext, useContext, useState, useEffect, useCallback, useRef } from 'react';
import { BleManager, State as BleState, Device, Characteristic } from 'react-native-ble-plx';

// PhotonStrike BLE service & characteristic UUIDs (custom 128-bit).
export const PS_SERVICE_UUID        = '0000ff50-0000-1000-8000-00805f9b34fb';
export const PS_CHAR_SCAN_UUID      = '0000ff51-0000-1000-8000-00805f9b34fb';
export const PS_CHAR_STATUS_UUID    = '0000ff52-0000-1000-8000-00805f9b34fb';
export const PS_CHAR_DFA_UUID       = '0000ff53-0000-1000-8000-00805f9b34fb';
export const PS_CHAR_CONTROL_UUID   = '0000ff54-0000-1000-8000-00805f9b34fb';
export const PS_CHAR_LOG_UUID       = '0000ff55-0000-1000-8000-00805f9b34fb';

// Fault classes (must match firmware board.h)
export enum FaultClass {
  NoChange = 0,
  SingleBit,
  SingleByte,
  MultiByte,
  Crash,
  Reset,
  Timeout,
}

export const FAULT_LABELS: Record<number, string> = {
  0: 'NO_CHANGE', 1: 'SINGLE_BIT', 2: 'SINGLE_BYTE',
  3: 'MULTI_BYTE', 4: 'CRASH', 5: 'RESET', 6: 'TIMEOUT',
};

// DFA result
export interface DfaResult {
  faultsCollected: number;
  uniqueBytes: number;
  key: number[];   // 16 bytes, -1 = unknown
}

// Scan status
export interface ScanStatus {
  shotsTotal: number;
  faultsTotal: number;
  code: number;       // 0xFFFFFFFF = started, 0xA11C = key recovered, 0xF1N15H = finished
  targetClkHz: number;
}

// Scan descriptor (must match firmware ps_scan_desc_t)
export interface ScanDescriptor {
  magic: number;
  trigSrc: number;
  trigEdge: number;
  trigPatternLo: number;
  trigPatternHi: number;
  trigMaskLo: number;
  trigMaskHi: number;
  powerThreshold: number;
  xStart: number; xStop: number; xStep: number;
  yStart: number; yStop: number; yStep: number;
  delayStartPs: number; delayStopPs: number; delayStepPs: number;
  widthStartNs: number; widthStopNs: number; widthStepNs: number;
  energyStart: number; energyStop: number; energyStep: number;
  shotsPerPoint: number;
  expectedHash: number;
  expectedLen: number;
  dfaMode: number;
}

const SCAN_MAGIC = 0x50534331;

interface BleContextValue {
  bleState: BleState;
  device: Device | null;
  connected: boolean;
  connecting: boolean;
  scanStatus: ScanStatus | null;
  dfaResult: DfaResult | null;
  faultGrid: Record<string, number>;   // "x,y" → fault class
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  sendScanDescriptor: (desc: ScanDescriptor) => Promise<void>;
  sendExpectedOutput: (bytes: number[]) => Promise<void>;
  startScan: () => Promise<void>;
  abortScan: () => Promise<void>;
  laserEnable: () => Promise<void>;
  laserDisable: () => Promise<void>;
  emergencyStop: () => Promise<void>;
  sendOutput: (bytes: number[]) => Promise<void>;
}

const BleContext = createContext<BleContextValue | undefined>(undefined);

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) throw new Error('useBle must be used within BleProvider');
  return ctx;
}

// ─── Encode a ScanDescriptor into the firmware binary struct ────────
function encodeScanDescriptor(d: ScanDescriptor): Uint8Array {
  const buf = new ArrayBuffer(56);
  const dv = new DataView(buf);
  let o = 0;
  dv.setUint32(o, d.magic, true);          o += 4;
  dv.setUint8 (o, d.trigSrc);               o += 1;
  dv.setUint8 (o, d.trigEdge);              o += 1;
  dv.setUint16(o, d.trigPatternLo, true);   o += 2;
  dv.setUint16(o, d.trigPatternHi, true);   o += 2;
  dv.setUint16(o, d.trigMaskLo, true);      o += 2;
  dv.setUint16(o, d.trigMaskHi, true);      o += 2;
  dv.setUint16(o, d.powerThreshold, true);  o += 2;
  dv.setUint16(o, d.xStart, true);          o += 2;
  dv.setUint16(o, d.xStop, true);           o += 2;
  dv.setUint16(o, d.xStep, true);           o += 2;
  dv.setUint16(o, d.yStart, true);          o += 2;
  dv.setUint16(o, d.yStop, true);           o += 2;
  dv.setUint16(o, d.yStep, true);           o += 2;
  dv.setUint32(o, d.delayStartPs, true);    o += 4;
  dv.setUint32(o, d.delayStopPs, true);     o += 4;
  dv.setUint32(o, d.delayStepPs, true);     o += 4;
  dv.setUint16(o, d.widthStartNs, true);    o += 2;
  dv.setUint16(o, d.widthStopNs, true);     o += 2;
  dv.setUint16(o, d.widthStepNs, true);     o += 2;
  dv.setUint16(o, d.energyStart, true);     o += 2;
  dv.setUint16(o, d.energyStop, true);      o += 2;
  dv.setUint16(o, d.energyStep, true);      o += 2;
  dv.setUint16(o, d.shotsPerPoint, true);   o += 2;
  dv.setUint32(o, d.expectedHash, true);    o += 4;
  dv.setUint16(o, d.expectedLen, true);     o += 2;
  dv.setUint8 (o, d.dfaMode);               o += 1;
  return new Uint8Array(buf);
}

// ─── Decode a status notification into ScanStatus ───────────────────
function decodeStatus(base64: string): ScanStatus {
  const raw = Buffer.from(base64, 'base64');
  const dv = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
  return {
    shotsTotal: dv.getUint32(0, true),
    faultsTotal: dv.getUint32(4, true),
    code: dv.getUint32(8, true),
    targetClkHz: dv.getUint32(12, true),
  };
}

// ─── Decode a DFA notification ──────────────────────────────────────
function decodeDfa(base64: string): DfaResult {
  const raw = Buffer.from(base64, 'base64');
  const dv = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
  const faults = dv.getUint8(0);
  const unique = dv.getUint8(1);
  const key: number[] = [];
  for (let i = 0; i < 16; i++) key.push(dv.getUint8(2 + i));
  return { faultsCollected: faults, uniqueBytes: unique, key };
}

// ─── Provider ────────────────────────────────────────────────────────
export function BleProvider({ children }: { children: React.ReactNode }) {
  const managerRef = useRef<BleManager | null>(null);
  const [bleState, setBleState] = useState<BleState>(BleState.Unknown);
  const [device, setDevice] = useState<Device | null>(null);
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [scanStatus, setScanStatus] = useState<ScanStatus | null>(null);
  const [dfaResult, setDfaResult] = useState<DfaResult | null>(null);
  const [faultGrid, setFaultGrid] = useState<Record<string, number>>({});

  // Initialize the BLE manager once.
  useEffect(() => {
    managerRef.current = new BleManager();
    const sub = managerRef.current.onStateChange((s) => setBleState(s), true);
    return () => { sub.remove(); managerRef.current?.destroy(); };
  }, []);

  const connect = useCallback(async () => {
    const mgr = managerRef.current;
    if (!mgr || bleState !== BleState.PoweredOn) return;
    setConnecting(true);
    try {
      const dev = await mgr.connectToDevice(PS_SERVICE_UUID, {
        autoConnect: true,
      });
      await dev.discoverAllServicesAndCharacteristics();
      setDevice(dev);
      setConnected(true);

      // Subscribe to status + DFA notifications.
      dev.setupNotification(PS_CHAR_STATUS_UUID).then(([sub]) => {
        sub.onData((c: Characteristic) => {
          if (!c.value) return;
          const st = decodeStatus(c.value);
          setScanStatus(st);
        });
      });
      dev.setupNotification(PS_CHAR_DFA_UUID).then(([sub]) => {
        sub.onData((c: Characteristic) => {
          if (!c.value) return;
          setDfaResult(decodeDfa(c.value));
        });
      });
    } finally {
      setConnecting(false);
    }
  }, [bleState]);

  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
      setDevice(null);
      setConnected(false);
    }
  }, [device]);

  const writeControl = useCallback(async (cmd: number) => {
    if (!device) return;
    await device.writeCharacteristicWithResponseForService(
      PS_SERVICE_UUID, PS_CHAR_CONTROL_UUID, Buffer.from([cmd]).toString('base64')
    );
  }, [device]);

  const sendScanDescriptor = useCallback(async (d: ScanDescriptor) => {
    if (!device) return;
    const bytes = encodeScanDescriptor({ ...d, magic: SCAN_MAGIC });
    await device.writeCharacteristicWithResponseForService(
      PS_SERVICE_UUID, PS_CHAR_SCAN_UUID,
      Buffer.from(bytes).toString('base64')
    );
  }, [device]);

  const sendExpectedOutput = useCallback(async (b: number[]) => {
    if (!device) return;
    await device.writeCharacteristicWithResponseForService(
      PS_SERVICE_UUID, PS_CHAR_LOG_UUID,
      Buffer.from(b).toString('base64')
    );
  }, [device]);

  const startScan    = useCallback(() => writeControl(0x01), [writeControl]);
  const abortScan    = useCallback(() => writeControl(0x02), [writeControl]);
  const laserEnable  = useCallback(() => writeControl(0x03), [writeControl]);
  const laserDisable = useCallback(() => writeControl(0x04), [writeControl]);
  const emergencyStop = useCallback(() => writeControl(0x05), [writeControl]);

  const sendOutput = useCallback(async (b: number[]) => {
    if (!device) return;
    // The device reads operator-supplied target output from the LOG char.
    await device.writeCharacteristicWithResponseForService(
      PS_SERVICE_UUID, PS_CHAR_LOG_UUID,
      Buffer.from(b).toString('base64')
    );
  }, [device]);

  const value: BleContextValue = {
    bleState, device, connected, connecting,
    scanStatus, dfaResult, faultGrid,
    connect, disconnect,
    sendScanDescriptor, sendExpectedOutput,
    startScan, abortScan,
    laserEnable, laserDisable, emergencyStop,
    sendOutput,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

// Buffer shim for react-native (base64 encode/decode)
class Buffer {
  static from(data: string | number[] | Uint8Array, encoding?: string): Buffer {
    if (typeof data === 'string' && encoding === 'base64') {
      // decode base64 → bytes
      const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
      const clean = data.replace(/=+$/, '');
      const bytes: number[] = [];
      let buf = 0, bits = 0;
      for (const c of clean) {
        const idx = chars.indexOf(c);
        if (idx < 0) continue;
        buf = (buf << 6) | idx;
        bits += 6;
        if (bits >= 8) { bits -= 8; bytes.push((buf >> bits) & 0xFF); }
      }
      return new Buffer(bytes);
    }
    if (data instanceof Uint8Array) return new Buffer(Array.from(data));
    return new Buffer(data as number[]);
  }
  private arr: number[];
  constructor(arr: number[]) { this.arr = arr; }
  get buffer() {
    const ab = new ArrayBuffer(this.arr.length);
    const u8 = new Uint8Array(ab);
    u8.set(this.arr);
    return ab;
  }
  get byteOffset() { return 0; }
  get byteLength() { return this.arr.length; }
  toString(encoding: string): string {
    if (encoding === 'base64') {
      const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
      let out = '';
      let buf = 0, bits = 0;
      for (const b of this.arr) {
        buf = (buf << 8) | b;
        bits += 8;
        while (bits >= 6) { bits -= 6; out += chars[(buf >> bits) & 0x3F]; }
      }
      if (bits > 0) out += chars[(buf << (6 - bits)) & 0x3F];
      while (out.length % 4) out += '=';
      return out;
    }
    return String.fromCharCode(...this.arr);
  }
}