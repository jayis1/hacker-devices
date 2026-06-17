/**
 * ForgeProbeService.ts — USB/Bluetooth Communication Service
 * Author: jayis1
 * License: MIT
 *
 * Handles all host-device communication with the Forge-Probe hardware.
 * Supports USB CDC (via USB serial bridge on STM32H743) and
 * BLE (via optional nRF52840 module).
 */

import {
  BleManager,
  Device as BleDevice,
  Characteristic,
} from 'react-native-ble-plx';
import AsyncStorage from '@react-native-async-storage/async-storage';

/* ─── Protocol Command Constants ─────────────────────────────────────────── */

export enum ForgeProbeCommand {
  IDENTIFY = 0x01,
  READ_MEMORY = 0x02,
  WRITE_MEMORY = 0x03,
  SCAN = 0x04,
  DUMP_FLASH = 0x05,
  FUZZ_MEM_AP = 0x06,
  RESET_TARGET = 0x07,
  HALT_TARGET = 0x08,
  RESUME_TARGET = 0x09,
  READ_REGISTER = 0x0A,
  BOUNDARY_SCAN = 0x0B,
  SET_CLOCK_SPEED = 0x0C,
  SET_PROTOCOL = 0x0D,
  GET_LOG = 0x0E,
  WRITE_FLASH = 0x0F,
}

export enum DebugProtocol {
  JTAG = 0,
  SWD = 1,
  CJTAG = 2,
  SWJ = 3,
}

export enum TargetArch {
  UNKNOWN = 0,
  CORTEX_M0 = 1,
  CORTEX_M3 = 2,
  CORTEX_M4 = 3,
  CORTEX_M7 = 4,
  CORTEX_M23 = 5,
  CORTEX_M33 = 6,
  CORTEX_R4 = 7,
  CORTEX_R5 = 8,
  CORTEX_A5 = 9,
  CORTEX_A7 = 10,
  CORTEX_A9 = 11,
  CORTEX_A15 = 12,
  CORTEX_A53 = 13,
  CORTEX_A72 = 14,
  RISCV = 15,
  CUSTOM = 16,
}

/* ─── Data Types ──────────────────────────────────────────────────────────── */

export interface TargetDescriptor {
  idcode: number;
  dpidr: number;
  protocol: DebugProtocol;
  architecture: TargetArch;
  targetId: number;
  memApBase: number;
  numAps: number;
  apBases: number[];
  irLength: number;
  tapCount: number;
  description: string;
  boundaryChainLen: number;
  manufacturerId: number;
  secureDebug: boolean;
  flashLocked: boolean;
  debugLevel: number;
}

export interface ScanResult {
  target: TargetDescriptor;
  memoryRegions: MemoryRegion[];
  timestamp: string;
}

export interface MemoryRegion {
  address: number;
  size: number;
  label: string;
  isReadable: boolean;
  isWritable: boolean;
}

export interface RegisterDump {
  r0: number;
  r1: number;
  r2: number;
  r3: number;
  r4: number;
  r5: number;
  r6: number;
  r7: number;
  r8: number;
  r9: number;
  r10: number;
  r11: number;
  r12: number;
  sp: number;
  lr: number;
  pc: number;
  xpsr: number;
}

/* ─── Service Class ────────────────────────────────────────────────────────── */

class ForgeProbeService {
  private bleManager: BleManager | null = null;
  private connectedDevice: BleDevice | null = null;
  private txCharacteristic: Characteristic | null = null;
  private rxCharacteristic: Characteristic | null = null;
  private usbSerial: any = null;
  private connectionType: 'usb' | 'ble' | null = null;
  private responseBuffer: Uint8Array = new Uint8Array(4096);
  private responseLength: number = 0;
  private pendingResolve: ((data: Uint8Array) => void) | null = null;

  /* ── Connection Management ─────────────────────────────────────────────── */

  async connectViaUSB(): Promise<boolean> {
    try {
      const usbSerialModule = require('react-native-usb-serial');
      this.usbSerial = new usbSerialModule.UsbSerial();

      // Forge-Probe USB Vendor/Product IDs
      await this.usbSerial.open({
        vendorId: 0x1234,
        productId: 0x0001,
        baudRate: 115200,
        dataBits: 8,
        stopBits: 1,
        parity: 'none',
      });

      this.connectionType = 'usb';
      console.log('ForgeProbe: Connected via USB');
      return true;
    } catch (error) {
      console.error('ForgeProbe: USB connection failed:', error);
      return false;
    }
  }

  async connectViaBLE(deviceId?: string): Promise<boolean> {
    try {
      this.bleManager = new BleManager();

      if (deviceId) {
        this.connectedDevice = await this.bleManager.connectToDevice(deviceId);
      } else {
        // Scan for Forge-Probe BLE service
        const devices = await this.bleManager.startDeviceScan(
          null,
          { services: ['180A'] },  // Device Information Service
          (error, device) => {
            if (error) return;
            if (device.name && device.name.includes('ForgeProbe')) {
              this.bleManager?.stopDeviceScan();
              this.connectViaBLE(device.id);
            }
          },
        );
      }

      if (!this.connectedDevice) return false;

      await this.connectedDevice.discoverAllServicesAndCharacteristics();
      const services = await this.connectedDevice.services();

      // Find our custom service
      const service = services.find(
        (s) => s.uuid === '0000FFE0-0000-1000-8000-00805F9B34FB',
      );
      if (!service) return false;

      const characteristics = await this.connectedDevice.characteristicsForService(
        service.id,
      );

      this.txCharacteristic = characteristics.find(
        (c) => c.uuid === '0000FFE1-0000-1000-8000-00805F9B34FB',
      );
      this.rxCharacteristic = characteristics.find(
        (c) => c.uuid === '0000FFE2-0000-1000-8000-00805F9B34FB',
      );

      await this.connectedDevice.monitorCharacteristicForService(
        service.uuid,
        this.rxCharacteristic!.uuid,
        (error, characteristic) => {
          if (characteristic?.value) {
            this.handleResponse(atob(characteristic.value));
          }
        },
      );

      this.connectionType = 'ble';
      console.log('ForgeProbe: Connected via BLE');
      return true;
    } catch (error) {
      console.error('ForgeProbe: BLE connection failed:', error);
      return false;
    }
  }

  disconnect(): void {
    if (this.connectionType === 'usb' && this.usbSerial) {
      this.usbSerial.close();
    } else if (this.connectionType === 'ble' && this.connectedDevice) {
      this.bleManager?.cancelDeviceConnection(this.connectedDevice.id);
    }
    this.connectionType = null;
    this.connectedDevice = null;
  }

  isConnected(): boolean {
    return this.connectionType !== null;
  }

  /* ── Low-Level Communication ───────────────────────────────────────────── */

  private async sendRaw(data: Uint8Array): Promise<void> {
    if (this.connectionType === 'usb' && this.usbSerial) {
      // USB-Serial bridge: prefix length, send as hex string
      const hexString = Array.from(data)
        .map((b) => b.toString(16).padStart(2, '0'))
        .join('');
      this.usbSerial.write(hexString + '\n');
    } else if (this.connectionType === 'ble' && this.txCharacteristic) {
      const base64 = btoa(String.fromCharCode(...data));
      await this.txCharacteristic.writeWithResponse(base64);
    }
  }

  async sendCommand(
    command: ForgeProbeCommand,
    payload?: Uint8Array,
  ): Promise<Uint8Array> {
    const header = new Uint8Array(1);
    header[0] = command;

    let packet: Uint8Array;
    if (payload) {
      packet = new Uint8Array(1 + payload.length);
      packet[0] = command;
      packet.set(payload, 1);
    } else {
      packet = header;
    }

    return new Promise((resolve) => {
      this.pendingResolve = resolve;
      this.sendRaw(packet);
    });
  }

  private handleResponse(raw: string): void {
    const data = new Uint8Array(raw.length);
    for (let i = 0; i < raw.length; i++) {
      data[i] = raw.charCodeAt(i);
    }

    if (this.pendingResolve) {
      this.pendingResolve(data);
      this.pendingResolve = null;
    }
  }

  /* ── High-Level API Commands ───────────────────────────────────────────── */

  async identify(): Promise<TargetDescriptor> {
    const response = await this.sendCommand(ForgeProbeCommand.IDENTIFY);

    return {
      protocol: response[3] as DebugProtocol,
      targetId: 0,  // Full parse depends on response format
      idcode: 0,
      dpidr: 0,
      architecture: TargetArch.UNKNOWN,
      memApBase: 0,
      numAps: 0,
      apBases: [],
      irLength: response[4],
      tapCount: response[5],
      description: String.fromCharCode(...Array.from(response.slice(8, 48))),
      boundaryChainLen: 0,
      manufacturerId: (response[1] << 8) | response[2],
      secureDebug: response[49] !== 0,
      flashLocked: response[50] !== 0,
      debugLevel: response[51],
    };
  }

  async scan(): Promise<TargetDescriptor> {
    const response = await this.sendCommand(ForgeProbeCommand.SCAN);
    // Parse scan response into target descriptor
    return this.parseTargetDescriptor(response);
  }

  async readMemory(address: number, length: number): Promise<Uint8Array> {
    const payload = new Uint8Array(8);
    payload.set([
      (address >> 24) & 0xFF,
      (address >> 16) & 0xFF,
      (address >> 8) & 0xFF,
      address & 0xFF,
      (length >> 24) & 0xFF,
      (length >> 16) & 0xFF,
      (length >> 8) & 0xFF,
      length & 0xFF,
    ]);

    return this.sendCommand(ForgeProbeCommand.READ_MEMORY, payload);
  }

  async dumpFlash(
    startAddress: number,
    length: number,
    onProgress?: (progress: number, total: number) => void,
  ): Promise<Uint8Array> {
    const payload = new Uint8Array(8);
    payload.set([
      (startAddress >> 24) & 0xFF,
      (startAddress >> 16) & 0xFF,
      (startAddress >> 8) & 0xFF,
      startAddress & 0xFF,
      (length >> 24) & 0xFF,
      (length >> 16) & 0xFF,
      (length >> 8) & 0xFF,
      length & 0xFF,
    ]);

    return this.sendCommand(ForgeProbeCommand.DUMP_FLASH, payload);
  }

  async haltTarget(): Promise<boolean> {
    await this.sendCommand(ForgeProbeCommand.HALT_TARGET);
    return true;
  }

  async resumeTarget(): Promise<boolean> {
    await this.sendCommand(ForgeProbeCommand.RESUME_TARGET);
    return true;
  }

  async readRegisters(): Promise<RegisterDump | null> {
    const registers: RegisterDump = {
      r0: 0, r1: 0, r2: 0, r3: 0,
      r4: 0, r5: 0, r6: 0, r7: 0,
      r8: 0, r9: 0, r10: 0, r11: 0,
      r12: 0, sp: 0, lr: 0, pc: 0, xpsr: 0,
    };

    const regNames: (keyof RegisterDump)[] = [
      'r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'r6', 'r7',
      'r8', 'r9', 'r10', 'r11', 'r12', 'sp', 'lr', 'pc', 'xpsr',
    ];

    for (let i = 0; i < regNames.length; i++) {
      const regPayload = new Uint8Array([i]);
      const response = await this.sendCommand(
        ForgeProbeCommand.READ_REGISTER,
        regPayload,
      );
      if (response.length >= 5) {
        registers[regNames[i]] =
          (response[2] << 0) |
          (response[3] << 8) |
          (response[4] << 16) |
          (response[5] << 24);
      }
    }

    return registers;
  }

  async boundaryScan(): Promise<number> {
    const response = await this.sendCommand(ForgeProbeCommand.BOUNDARY_SCAN);
    if (response.length >= 4) {
      return (
        (response[0] << 0) |
        (response[1] << 8) |
        (response[2] << 16) |
        (response[3] << 24)
      );
    }
    return 0;
  }

  async setProtocol(protocol: DebugProtocol): Promise<void> {
    const payload = new Uint8Array([protocol]);
    await this.sendCommand(ForgeProbeCommand.SET_PROTOCOL, payload);
  }

  async setClockSpeed(hz: number): Promise<void> {
    const payload = new Uint8Array(4);
    payload.set([
      (hz >> 24) & 0xFF,
      (hz >> 16) & 0xFF,
      (hz >> 8) & 0xFF,
      hz & 0xFF,
    ]);
    await this.sendCommand(ForgeProbeCommand.SET_CLOCK_SPEED, payload);
  }

  async fuzzMemory(): Promise<string[]> {
    const response = await this.sendCommand(ForgeProbeCommand.FUZZ_MEM_AP);
    const text = String.fromCharCode(...Array.from(response));
    return text.split('\n').filter((l) => l.length > 0);
  }

  async resetTarget(): Promise<void> {
    await this.sendCommand(ForgeProbeCommand.RESET_TARGET);
  }

  /* ── Parsing Helpers ───────────────────────────────────────────────────── */

  private parseTargetDescriptor(data: Uint8Array): TargetDescriptor {
    return {
      idcode:
        (data[0] << 0) |
        (data[1] << 8) |
        (data[2] << 16) |
        (data[3] << 24),
      dpidr:
        (data[4] << 0) |
        (data[5] << 8) |
        (data[6] << 16) |
        (data[7] << 24),
      protocol: data[8] as DebugProtocol,
      architecture: data[9] as TargetArch,
      targetId:
        (data[10] << 0) |
        (data[11] << 8) |
        (data[12] << 16) |
        (data[13] << 24),
      memApBase:
        (data[14] << 0) |
        (data[15] << 8) |
        (data[16] << 16) |
        (data[17] << 24),
      numAps: data[18],
      apBases: [],
      irLength: data[20],
      tapCount: data[21],
      description: String.fromCharCode(...Array.from(data.slice(22, 70))),
      boundaryChainLen:
        (data[70] << 0) |
        (data[71] << 8) |
        (data[72] << 16) |
        (data[73] << 24),
      manufacturerId: (data[74] << 8) | data[75],
      secureDebug: data[76] !== 0,
      flashLocked: data[77] !== 0,
      debugLevel: data[78],
    };
  }
}

export const forgeProbeService = new ForgeProbeService();
export default forgeProbeService;