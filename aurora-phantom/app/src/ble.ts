// src/ble.ts — BLE service module for Aurora-Phantom
// Author: jayis1   License: GPL-2.0
//
// Wraps react-native-ble-plx to connect to the Aurora-Phantom device,
// subscribe to the DATA characteristic notify stream, and send commands
// over the CMD characteristic. Exposes a small typed API.

import { BleManager, Device, Subscription } from 'react-native-ble-plx';

export const AURORA_SERVICE = 'a80d0001-5b3e-4c92-9f1d-0c1b2a3c4d5e';
export const CMD_CHAR      = 'a80d0002-5b3e-4c92-9f1d-0c1b2a3c4d5e';
export const DATA_CHAR     = 'a80d0003-5b3e-4c92-9f1d-0c1b2a3c4d5e';

export type ConnectionState = 'idle' | 'scanning' | 'connecting' | 'connected' | 'disconnected';

export interface FramePacket {
  seq: number;
  kind: 'header' | 'data' | 'trailer';
  bytes: Uint8Array;
}

class AuroraBLE {
  private manager: BleManager;
  private device: Device | null = null;
  private dataSub: Subscription | null = null;
  private frameCb: ((p: FramePacket) => void) | null = null;

  state: ConnectionState = 'idle';

  constructor() {
    this.manager = new BleManager();
  }

  /** Scan for devices advertising the Aurora service. */
  scan(timeoutMs: number = 5000): Promise<Device[]> {
    return new Promise((resolve) => {
      const found: Device[] = [];
      this.state = 'scanning';
      this.manager.startDeviceScan([AURORA_SERVICE], null, (err, dev) => {
        if (err) { this.manager.stopDeviceScan(); resolve([]); return; }
        if (dev && !found.find(d => d.id === dev.id)) found.push(dev);
      });
      setTimeout(() => {
        this.manager.stopDeviceScan();
        this.state = 'idle';
        resolve(found);
      }, timeoutMs);
    });
  }

  /** Connect + discover services + subscribe to DATA notifications. */
  async connect(deviceId: string): Promise<void> {
    this.state = 'connecting';
    this.device = await this.manager.connectToDevice(deviceId);
    await this.device.discoverAllServicesAndCharacteristics();
    this.dataSub = this.device.monitorCharacteristicForService(
      AURORA_SERVICE, DATA_CHAR, (err, char) => {
        if (err || !char?.value) return;
        const raw = base64ToBytes(char.value);
        const pkt = parsePacket(raw);
        if (pkt) this.frameCb?.(pkt);
      });
    this.state = 'connected';
  }

  async disconnect(): Promise<void> {
    this.dataSub?.remove();
    this.dataSub = null;
    if (this.device) await this.device.cancelConnection();
    this.device = null;
    this.state = 'disconnected';
  }

  /** Send a command (JSON string) over the CMD characteristic. */
  async sendCommand(json: string): Promise<void> {
    if (!this.device) throw new Error('not connected');
    const bytes = stringToBase64(json);
    await this.device.writeCharacteristicWithResponseForService(
      AURORA_SERVICE, CMD_CHAR, bytes);
  }

  onFrame(cb: (p: FramePacket) => void) { this.frameCb = cb; }
  isConnected() { return this.state === 'connected'; }
}

function base64ToBytes(b64: string): Uint8Array {
  // Minimal base64 decode (production: use a lib)
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const clean = b64.replace(/=+$/, '');
  const out: number[] = [];
  let buf = 0, bits = 0;
  for (const c of clean) {
    const idx = chars.indexOf(c);
    if (idx < 0) continue;
    buf = (buf << 6) | idx;
    bits += 6;
    if (bits >= 8) { bits -= 8; out.push((buf >> bits) & 0xff); }
  }
  return new Uint8Array(out);
}

function stringToBase64(s: string): string {
  // For small command payloads this is sufficient.
  const bytes = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) bytes[i] = s.charCodeAt(i);
  let bin = '';
  for (const b of bytes) bin += String.fromCharCode(b);
  // btoa equivalent
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let out = '';
  for (let i = 0; i < bin.length; i += 3) {
    const a = bin.charCodeAt(i);
    const b = i + 1 < bin.length ? bin.charCodeAt(i + 1) : 0;
    const c = i + 2 < bin.length ? bin.charCodeAt(i + 2) : 0;
    out += chars[a >> 2];
    out += chars[((a & 3) << 4) | (b >> 4)];
    out += i + 1 < bin.length ? chars[((b & 15) << 2) | (c >> 6)] : '=';
    out += i + 2 < bin.length ? chars[c & 63] : '=';
  }
  return out;
}

function parsePacket(raw: Uint8Array): FramePacket | null {
  if (raw.length < 3) return null;
  const kind = raw[0];
  const seq = raw[1] | (raw[2] << 8);
  if (kind === 0xA5) {
    return { seq, kind: raw.length > 3 ? 'header' : 'trailer', bytes: raw.slice(3) };
  } else if (kind === 0xAA) {
    return { seq, kind: 'data', bytes: raw.slice(3) };
  }
  return null;
}

export const ble = new AuroraBLE();