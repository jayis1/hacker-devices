/**
 * Tactile-Phantom — Companion App
 * src/ble.ts — BLE manager for Tactile-Phantom device communication
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Manages the BLE connection to the Tactile-Phantom implant via the
 * ESP32-C3 module. Handles GATT characteristic reads/writes and
 * notification subscriptions for event streaming.
 *
 * BLE GATT service: 0000tp01-0000-1000-8000-00805f9b34fb
 * Characteristics:
 *   tp01-01 — Event stream (notify): decoded touch events
 *   tp01-02 — Command (write): injection commands
 *   tp01-03 — Status (read/notify): bus state, battery, counters
 *   tp01-04 — Layout (write): keyboard layout configuration
 *   tp01-05 — Storage (read): SD card log data
 *   tp01-06 — Firmware (write): OTA update data
 */

import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';

// BLE service and characteristic UUIDs
const SERVICE_UUID = '0000tp01-0000-1000-8000-00805f9b34fb';
const CHAR_EVENT   = '0000tp01-0001-1000-8000-00805f9b34fb';
const CHAR_COMMAND = '0000tp01-0002-1000-8000-00805f9b34fb';
const CHAR_STATUS  = '0000tp01-0003-1000-8000-00805f9b34fb';
const CHAR_LAYOUT  = '0000tp01-0004-1000-8000-00805f9b34fb';
const CHAR_STORAGE = '0000tp01-0005-1000-8000-00805f9b34fb';
const CHAR_FIRMWARE= '0000tp01-0006-1000-8000-00805f9b34fb';

export interface TouchEvent {
  timestamp: number;
  type: number;      // TP_EV_TOUCH, TP_EV_GESTURE, etc.
  vendor: number;
  fingerCount: number;
  fingers: Finger[];
  gestureId?: number;
  regAddr?: number;
  regLen?: number;
}

export interface Finger {
  id: number;
  x: number;
  y: number;
  pressure: number;
  flags: number;
}

export interface BusStatus {
  mode: number;       // 0=auto, 1=I2C, 2=SPI
  vendor: number;
  i2cAddr: number;
  clockHz: number;
  xResolution: number;
  yResolution: number;
  attached: boolean;
  capturing: boolean;
  injectArmed: boolean;
  totalTransactions: number;
  totalEvents: number;
  batteryMv: number;
}

export interface InjectCommand {
  type: number;       // TP_INJ_TAP, TP_INJ_SWIPE, etc.
  x1: number;
  y1: number;
  x2: number;
  y2: number;
  durationMs: number;
  fingerCount: number;
}

export type EventCallback = (event: TouchEvent) => void;
export type StatusCallback = (status: BusStatus) => void;

class TactilePhantomBLE {
  private manager: BleManager;
  private device: Device | null = null;
  private eventCallback: EventCallback | null = null;
  private statusCallback: StatusCallback | null = null;
  private connected: boolean = false;

  constructor() {
    this.manager = new BleManager();
  }

  // Request BLE permissions (Android)
  async requestPermissions(): Promise<boolean> {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        (v) => v === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;  // iOS handles permissions at system level
  }

  // Scan for Tactile-Phantom devices
  async scanForDevices(timeoutMs: number = 10000): Promise<Device[]> {
    const devices: Device[] = [];
    return new Promise((resolve) => {
      this.manager.startDeviceScan([SERVICE_UUID], null, (error, device) => {
        if (error) {
          console.error('[BLE] Scan error:', error);
          resolve(devices);
          return;
        }
        if (device && !devices.find((d) => d.id === device.id)) {
          devices.push(device);
        }
      });
      setTimeout(() => {
        this.manager.stopDeviceScan();
        resolve(devices);
      }, timeoutMs);
    });
  }

  // Connect to a device
  async connect(deviceId: string): Promise<boolean> {
    try {
      this.device = await this.manager.connectToDevice(deviceId);
      await this.device.discoverAllServicesAndCharacteristics();
      this.connected = true;

      // Subscribe to event notifications
      await this.device.setupNotifications(
        this.findCharacteristic(CHAR_EVENT),
        (data, error) => {
          if (error) {
            console.error('[BLE] Event notification error:', error);
            return;
          }
          if (data?.value && this.eventCallback) {
            const event = this.parseEvent(data.value);
            this.eventCallback(event);
          }
        }
      );

      // Subscribe to status notifications
      await this.device.setupNotifications(
        this.findCharacteristic(CHAR_STATUS),
        (data, error) => {
          if (error) {
            console.error('[BLE] Status notification error:', error);
            return;
          }
          if (data?.value && this.statusCallback) {
            const status = this.parseStatus(data.value);
            this.statusCallback(status);
          }
        }
      );

      return true;
    } catch (error) {
      console.error('[BLE] Connect failed:', error);
      this.connected = false;
      return false;
    }
  }

  // Disconnect
  async disconnect(): Promise<void> {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.connected = false;
    }
  }

  // Find a characteristic by UUID
  private findCharacteristic(uuid: string): Characteristic | null {
    if (!this.device) return null;
    const services = this.device.services();
    for (const service of services) {
      const char = service.characteristics().find((c) => c.uuid === uuid);
      if (char) return char;
    }
    return null;
  }

  // Register event callback
  onEvent(cb: EventCallback): void {
    this.eventCallback = cb;
  }

  // Register status callback
  onStatus(cb: StatusCallback): void {
    this.statusCallback = cb;
  }

  // Send an injection command
  async sendInjectCommand(cmd: InjectCommand): Promise<boolean> {
    if (!this.device || !this.connected) return false;
    const data = this.serializeInjectCommand(cmd);
    const char = this.findCharacteristic(CHAR_COMMAND);
    if (!char) return false;
    await char.writeWithResponse(data);
    return true;
  }

  // Send keyboard layout
  async sendLayout(layoutData: Uint8Array): Promise<boolean> {
    if (!this.device || !this.connected) return false;
    const char = this.findCharacteristic(CHAR_LAYOUT);
    if (!char) return false;
    // BLE max MTU is ~512 bytes; chunk if needed
    const chunks = this.chunkData(layoutData, 200);
    for (const chunk of chunks) {
      await char.writeWithResponse(chunk);
    }
    return true;
  }

  // Arm/disarm injection
  async armInject(armed: boolean): Promise<boolean> {
    if (!this.device || !this.connected) return false;
    const data = new Uint8Array([armed ? 1 : 0]);
    const char = this.findCharacteristic(CHAR_COMMAND);
    if (!char) return false;
    // Send arm command (type 0x13)
    const msg = new Uint8Array([0x13, armed ? 1 : 0]);
    await char.writeWithResponse(msg);
    return true;
  }

  // Parse a touch event from BLE notification data
  private parseEvent(base64Data: string): TouchEvent {
    const bytes = this.base64ToBytes(base64Data);
    const view = new DataView(bytes.buffer);
    let pos = 0;

    const timestamp = view.getUint32(pos, true); pos += 4;
    const type = bytes[pos++];
    const vendor = bytes[pos++];
    const fingerCount = bytes[pos++];

    const fingers: Finger[] = [];
    for (let i = 0; i < fingerCount && i < 10; i++) {
      const id = bytes[pos++];
      const x = view.getUint16(pos, true); pos += 2;
      const y = view.getUint16(pos, true); pos += 2;
      const pressure = bytes[pos++];
      const flags = bytes[pos++];
      fingers.push({ id, x, y, pressure, flags });
    }

    let gestureId: number | undefined;
    if (type === 3 && pos < bytes.length) {  // TP_EV_GESTURE
      gestureId = view.getUint16(pos, true); pos += 2;
    }

    return { timestamp, type, vendor, fingerCount, fingers, gestureId };
  }

  // Parse status from BLE notification
  private parseStatus(base64Data: string): BusStatus {
    const bytes = this.base64ToBytes(base64Data);
    const view = new DataView(bytes.buffer);
    let pos = 0;

    const mode = bytes[pos++];
    const vendor = bytes[pos++];
    const i2cAddr = bytes[pos++];
    const clockHz = view.getUint32(pos, true); pos += 4;
    const xResolution = view.getUint16(pos, true); pos += 2;
    const yResolution = view.getUint16(pos, true); pos += 2;
    const attached = bytes[pos++] !== 0;
    const capturing = bytes[pos++] !== 0;
    const injectArmed = bytes[pos++] !== 0;
    const totalTransactions = view.getUint32(pos, true); pos += 4;
    const totalEvents = view.getUint32(pos, true); pos += 4;
    const batteryMv = view.getUint16(pos, true);

    return {
      mode, vendor, i2cAddr, clockHz, xResolution, yResolution,
      attached, capturing, injectArmed, totalTransactions,
      totalEvents, batteryMv,
    };
  }

  // Serialize an injection command to binary
  private serializeInjectCommand(cmd: InjectCommand): Uint8Array {
    const buf = new Uint8Array(16);
    const view = new DataView(buf.buffer);
    buf[0] = cmd.type;
    view.setUint16(1, cmd.x1, true);
    view.setUint16(3, cmd.y1, true);
    view.setUint16(5, cmd.x2, true);
    view.setUint16(7, cmd.y2, true);
    view.setUint16(9, cmd.durationMs, true);
    buf[11] = cmd.fingerCount;
    return buf;
  }

  // Utility: base64 to Uint8Array
  private base64ToBytes(base64: string): Uint8Array {
    const chars = atob(base64);
    const bytes = new Uint8Array(chars.length);
    for (let i = 0; i < chars.length; i++) {
      bytes[i] = chars.charCodeAt(i);
    }
    return bytes;
  }

  // Utility: chunk data for BLE writes
  private chunkData(data: Uint8Array, chunkSize: number): Uint8Array[] {
    const chunks: Uint8Array[] = [];
    for (let i = 0; i < data.length; i += chunkSize) {
      chunks.push(data.slice(i, i + chunkSize));
    }
    return chunks;
  }

  isConnected(): boolean {
    return this.connected;
  }
}

// Singleton instance
export const bleManager = new TactilePhantomBLE();