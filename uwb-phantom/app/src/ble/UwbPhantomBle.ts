/**
 * UwbPhantomBle.ts — BLE GATT client for the UWB-PHANTOM hardware.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Wraps `react-native-ble-plx` with a small typed API tuned to the four
 * characteristics documented in the README:
 *
 *   Service  : 0000FEA5-0000-1000-8000-00805F9B34FB
 *     cmd     (write)   — JSON command
 *     event   (notify)  — JSON event stream
 *     log     (notify)  — raw PCAP chunks
 *     target  (r/w)     — authorised-target blob
 *
 * All command/response and event parsing happens here so the screen
 * components stay declarative.
 */

import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import base64 from 'base64-js';

export const UWB_PHANTOM_SERVICE_UUID =
  '0000fea5-0000-1000-8000-00805f9b34fb';
const CMD_UUID   = '0000fea6-0000-1000-8000-00805f9b34fb';
const EVT_UUID   = '0000fea7-0000-1000-8000-00805f9b34fb';
const LOG_UUID   = '0000fea8-0000-1000-8000-00805f9b34fb';
const TGT_UUID   = '0000fea9-0000-1000-8000-00805f9b34fb';

export interface BleDevice {
  id: string;
  name: string;
  rssi: number;
}

export interface UwbEvent {
  v: string;             /* verb */
  [k: string]: any;
}

export interface TargetRecord {
  eui: Uint8Array;       /* 8 bytes */
  label: string;          /* ≤ 24 chars */
  hmac: Uint8Array;       /* 32 bytes */
  nonce: bigint;
}

type EventListener = (e: UwbEvent) => void;
type LogListener   = (chunk: Uint8Array) => void;

export class UwbPhantomBle {
  private manager: BleManager;
  private device: Device | null = null;
  private evtListeners: Set<EventListener> = new Set();
  private logListeners: Set<LogListener> = new Set();

  constructor() {
    this.manager = new BleManager();
  }

  /* ---- Scanning -------------------------------------- */

  async scan(timeoutMs = 5000): Promise<BleDevice[]> {
    const found: BleDevice[] = [];
    this.manager.startDeviceScan([UWB_PHANTOM_SERVICE_UUID], null,
      (err, d) => {
        if (err || !d || !d.name) return;
        if (d.name.startsWith('UWB-PHANTOM-')) {
          found.push({ id: d.id, name: d.name, rssi: d.rssi ?? -100 });
        }
      });
    await sleep(timeoutMs);
    this.manager.stopDeviceScan();
    return found;
  }

  /* ---- Connection + GATT ---------------------------- */

  async connect(deviceId: string): Promise<void> {
    this.device = await this.manager.connectToDevice(deviceId);
    await this.device.discoverAllServicesAndCharacteristics();
    /* Subscribe to event + log notifications */
    await this.device.setupNotifications(EVT_UUID, (err, c) => {
      if (err || !c?.value) return;
      const json = decodeBase64ToJson(c.value);
      const evt: UwbEvent = JSON.parse(json);
      this.evtListeners.forEach(fn => fn(evt));
    });
    await this.device.setupNotifications(LOG_UUID, (err, c) => {
      if (err || !c?.value) return;
      const chunk = base64.toByteArray(c.value);
      this.logListeners.forEach(fn => fn(chunk));
    });
  }

  async disconnect(): Promise<void> {
    if (this.device) await this.device.cancelConnection();
    this.device = null;
  }

  /* ---- Commands ------------------------------------- */

  async sendCmd(obj: Record<string, any>): Promise<void> {
    if (!this.device) throw new Error('not connected');
    const json = JSON.stringify(obj);
    const b64  = encodeJsonToBase64(json);
    await this.device.writeCharacteristicWithResponseForDevice(
      this.device.id, UWB_PHANTOM_SERVICE_UUID, CMD_UUID, b64);
  }

  /* ---- Targets ------------------------------------- */

  async loadTarget(t: TargetRecord): Promise<void> {
    if (!this.device) throw new Error('not connected');
    /* Pack: 8B eui + 24B label + 8B nonce + 32B hmac = 72 bytes */
    const buf = new Uint8Array(72);
    buf.set(t.eui, 0);
    for (let i = 0; i < 24; i++)
      buf[8 + i] = t.label.charCodeAt(i) || 0;
    /* nonce as little-endian 8 bytes */
    let n = t.nonce;
    for (let i = 0; i < 8; i++) { buf[32 + i] = Number(n & 0xffn); n >>= 8n; }
    buf.set(t.hmac, 40);
    const b64 = base64.fromByteArray(buf);
    await this.device.writeCharacteristicWithResponseForDevice(
      this.device.id, UWB_PHANTOM_SERVICE_UUID, TGT_UUID, b64);
  }

  /* ---- Listeners ----------------------------------- */

  onEvent(fn: EventListener): () => void {
    this.evtListeners.add(fn);
    return () => this.evtListeners.delete(fn);
  }

  onLog(fn: LogListener): () => void {
    this.logListeners.add(fn);
    return () => this.logListeners.delete(fn);
  }

  isConnected(): boolean { return this.device != null; }
}

/* ---- Helpers ---------------------------------------- */

function sleep(ms: number): Promise<void> {
  return new Promise(r => setTimeout(r, ms));
}

function decodeBase64ToJson(b64: string): string {
  const bytes = base64.toByteArray(b64);
  return new TextDecoder().decode(bytes);
}

function encodeJsonToBase64(json: string): string {
  const bytes = new TextEncoder().encode(json);
  return base64.fromByteArray(bytes);
}

/* ---- React context bridge --------------------------- */

import { createContext, useContext } from 'react';
export const BleContext = createContext<{
  device: BleDevice | null;
  setDevice: (d: BleDevice | null) => void;
}>({ device: null, setDevice: () => {} });
export const useBle = () => useContext(BleContext);

/* Singleton for non-React modules */
export const ble = new UwbPhantomBle();