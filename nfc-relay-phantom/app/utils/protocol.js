/**
 * NFC Relay Phantom — BLE Protocol Manager
 * Handles BLE communication between the phone app and the Phantom device
 *
 * GATT Service Layout:
 *   0xFFF0 - NFC Control Service
 *     0xFFF1 - Mode select (R/W)
 *     0xFFF2 - Protocol select (R/W)
 *     0xFFF3 - Start/Stop (R/W)
 *   0xFFF4 - NFC Data Service
 *     0xFFF5 - UID read (Notify)
 *     0xFFF6 - Frame data (Notify)
 *     0xFFF7 - Send frame (Write)
 *   0xFFF8 - RFID 125 Service
 *     0xFFF9 - Card ID read (Notify)
 *     0xFFFA - Write T5577 (Write)
 *   0xFFFB - Relay Service
 *     0xFFFC - Relay data (Notify + Write)
 *     0xFFFD - Relay control (R/W)
 *   0x180A - Device Info Service (standard)
 *   0x180F - Battery Service
 *     0x2A19 - Battery level (Notify)
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

import { BleManager } from 'react-native-ble-plx';
import { Platform } from 'react-native';

// BLE Service and Characteristic UUIDs
const SERVICE_NFC_CONTROL  = '0000fff0-0000-1000-8000-00805f9b34fb';
const CHAR_MODE_SELECT     = '0000fff1-0000-1000-8000-00805f9b34fb';
const CHAR_PROTOCOL_SELECT = '0000fff2-0000-1000-8000-00805f9b34fb';
const CHAR_START_STOP      = '0000fff3-0000-1000-8000-00805f9b34fb';

const SERVICE_NFC_DATA     = '0000fff4-0000-1000-8000-00805f9b34fb';
const CHAR_UID_READ        = '0000fff5-0000-1000-8000-00805f9b34fb';
const CHAR_FRAME_DATA      = '0000fff6-0000-1000-8000-00805f9b34fb';
const CHAR_SEND_FRAME      = '0000fff7-0000-1000-8000-00805f9b34fb';

const SERVICE_RFID_125     = '0000fff8-0000-1000-8000-00805f9b34fb';
const CHAR_CARD_ID_READ    = '0000fff9-0000-1000-8000-00805f9b34fb';
const CHAR_WRITE_T5577     = '0000fffa-0000-1000-8000-00805f9b34fb';

const SERVICE_RELAY        = '0000fffb-0000-1000-8000-00805f9b34fb';
const CHAR_RELAY_DATA      = '0000fffc-0000-1000-8000-00805f9b34fb';
const CHAR_RELAY_CONTROL   = '0000fffd-0000-1000-8000-00805f9b34fb';

const SERVICE_BATTERY      = '0000180f-0000-1000-8000-00805f9b34fb';
const CHAR_BATTERY_LEVEL   = '00002a19-0000-1000-8000-00805f9b34fb';

const DEVICE_NAME_PREFIX = 'Phantom';

export class PhantomBLEManager {
  constructor() {
    this.bleManager = new BleManager();
    this.device = null;
    this.connected = false;
    this.connectionListeners = new Set();
    this.nfcListeners = new Set();
    this.rfidListeners = new Set();
    this.relayListeners = new Set();
  }

  // ========== Connection Management ==========

  onConnectionChange(callback) {
    this.connectionListeners.add(callback);
    return () => this.connectionListeners.delete(callback);
  }

  _notifyConnection(connected) {
    this.connected = connected;
    this.connectionListeners.forEach(cb => cb(connected));
  }

  async scanAndConnect() {
    try {
      const device = await this.bleManager.connectToDevice(
        await this._findDevice()
      );
      this.device = device;
      this._notifyConnection(true);

      // Discover all services and characteristics
      await device.discoverAllServicesAndCharacteristics();

      // Subscribe to notifications
      await this._subscribeNotifications();

      return device;
    } catch (error) {
      console.error('BLE connect error:', error);
      this._notifyConnection(false);
      throw error;
    }
  }

  async _findDevice() {
    return new Promise((resolve, reject) => {
      const subscription = this.bleManager.onStateChange((state) => {
        if (state === 'PoweredOn') {
          this.bleManager.startDeviceScan(
            [SERVICE_NFC_CONTROL, SERVICE_NFC_DATA],
            null,
            (error, device) => {
              if (error) {
                reject(error);
                return;
              }
              if (device.name?.startsWith(DEVICE_NAME_PREFIX)) {
                this.bleManager.stopDeviceScan();
                subscription.remove();
                resolve(device.id);
              }
            }
          );
        }
      }, true);
    });
  }

  async disconnect() {
    if (this.device) {
      await this.bleManager.cancelDeviceConnection(this.device.id);
      this.device = null;
      this._notifyConnection(false);
    }
  }

  // ========== Notification Subscriptions ==========

  async _subscribeNotifications() {
    if (!this.device) return;

    // NFC UID notifications
    await this.bleManager.onCharacteristicValueChangeForDevice(
      this.device.id,
      SERVICE_NFC_DATA,
      CHAR_UID_READ,
      (error, characteristic) => {
        if (error) return;
        const data = this._parseNFCData(characteristic.value);
        this.nfcListeners.forEach(cb => cb(data));
      }
    );

    // RFID 125 kHz card ID notifications
    await this.bleManager.onCharacteristicValueChangeForDevice(
      this.device.id,
      SERVICE_RFID_125,
      CHAR_CARD_ID_READ,
      (error, characteristic) => {
        if (error) return;
        const data = this._parseRFIDData(characteristic.value);
        this.rfidListeners.forEach(cb => cb(data));
      }
    );

    // Relay data notifications
    await this.bleManager.onCharacteristicValueChangeForDevice(
      this.device.id,
      SERVICE_RELAY,
      CHAR_RELAY_DATA,
      (error, characteristic) => {
        if (error) return;
        const data = this._parseRelayData(characteristic.value);
        this.relayListeners.forEach(cb => cb(data));
      }
    );
  }

  // ========== Command Interface ==========

  async sendCommand(service, command, params) {
    if (!this.device) throw new Error('Not connected');

    let charUUID;
    switch (service) {
      case 'NFC':
        if (command === 'READER_START' || command === 'READER_STOP' ||
            command === 'EMULATE_START' || command === 'EMULATE_STOP' ||
            command === 'SNIFFER_START' || command === 'SNIFFER_STOP') {
          charUUID = CHAR_START_STOP;
        }
        if (command === 'READER_START') {
          // Set protocol first
          await this._writeCharacteristic(CHAR_PROTOCOL_SELECT,
            this._protocolToBytes(params.protocol));
        }
        break;
      case 'RFID125':
        if (command === 'READ_START' || command === 'READ_STOP') {
          charUUID = CHAR_START_STOP;
        } else if (command === 'WRITE_T5577') {
          charUUID = CHAR_WRITE_T5577;
        }
        break;
      case 'RELAY':
        if (command === 'START' || command === 'STOP') {
          charUUID = CHAR_RELAY_CONTROL;
        }
        break;
      case 'SETTINGS':
        // Settings commands go through NFC Control service
        charUUID = CHAR_MODE_SELECT;
        break;
    }

    if (charUUID) {
      const payload = this._encodeCommand(service, command, params);
      await this._writeCharacteristic(charUUID, payload);
    }
  }

  async _writeCharacteristic(charUUID, data) {
    if (!this.device) throw new Error('Not connected');
    await this.bleManager.writeCharacteristicWithResponseForDevice(
      this.device.id,
      SERVICE_NFC_CONTROL,
      charUUID,
      data
    );
  }

  // ========== Battery ==========

  async readBattery() {
    if (!this.device) return 0;
    try {
      const characteristic = await this.bleManager.readCharacteristicForDevice(
        this.device.id,
        SERVICE_BATTERY,
        CHAR_BATTERY_LEVEL
      );
      return parseInt(characteristic.value, 10);
    } catch {
      return 0;
    }
  }

  // ========== Data Listeners ==========

  onNFCData(callback) {
    this.nfcListeners.add(callback);
    return () => this.nfcListeners.delete(callback);
  }

  onRFID125Data(callback) {
    this.rfidListeners.add(callback);
    return () => this.rfidListeners.delete(callback);
  }

  onRelayData(callback) {
    this.relayListeners.add(callback);
    return () => this.relayListeners.delete(callback);
  }

  // ========== Data Parsing ==========

  _parseNFCData(base64) {
    const bytes = this._base64ToBytes(base64);
    const typeByte = bytes[0];

    if (typeByte === 0x01) {
      // Tag read notification
      const uidLen = bytes[1];
      const uid = Array.from(bytes.slice(2, 2 + uidLen))
        .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
        .join(':');
      const atqa = bytes.slice(2 + uidLen, 4 + uidLen)
        .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
        .join('');
      const sak = bytes[4 + uidLen];

      return {
        type: 'tag',
        uid,
        atqa,
        sak: sak.toString(16).toUpperCase().padStart(2, '0'),
        protocol: this._protocolFromByte(bytes[5 + uidLen]),
        timestamp: new Date().toISOString(),
      };
    } else if (typeByte === 0x02) {
      // Frame data notification
      const dir = bytes[1] === 0x01 ? 'PCD' : 'PICC';
      const frameLen = (bytes[2] << 8) | bytes[3];
      const hex = Array.from(bytes.slice(4, 4 + frameLen))
        .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
        .join(' ');

      return {
        type: 'frame',
        direction: dir,
        hex,
        length: frameLen,
      };
    }

    return { type: 'unknown' };
  }

  _parseRFIDData(base64) {
    const bytes = this._base64ToBytes(base64);
    const protoByte = bytes[0];
    const bitCount = (bytes[1] << 8) | bytes[2];
    const dataLen = bytes[3];
    const uidHex = Array.from(bytes.slice(4, 4 + dataLen))
      .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
      .join(':');

    return {
      protocol: this._rfidProtocolFromByte(protoByte),
      uid_hex: uidHex,
      bit_count: bitCount,
      facility_code: (bytes[4 + dataLen] << 8) | bytes[5 + dataLen],
      card_number: (bytes[6 + dataLen] << 24) | (bytes[7 + dataLen] << 16) |
                   (bytes[8 + dataLen] << 8) | bytes[9 + dataLen],
    };
  }

  _parseRelayData(base64) {
    const bytes = this._base64ToBytes(base64);
    return {
      peerConnected: bytes[0] === 1,
      latency: (bytes[1] << 8) | bytes[2],
      frames: (bytes[3] << 24) | (bytes[4] << 16) | (bytes[5] << 8) | bytes[6],
    };
  }

  // ========== Encoding Helpers ==========

  _encodeCommand(service, command, params) {
    const cmdMap = {
      'READER_START':  0x01,
      'READER_STOP':   0x02,
      'EMULATE_START': 0x03,
      'EMULATE_STOP':  0x04,
      'SNIFFER_START': 0x05,
      'SNIFFER_STOP':  0x06,
      'READ_START':    0x07,
      'READ_STOP':     0x08,
      'WRITE_T5577':  0x09,
      'RELAY_START':  0x0A,
      'RELAY_STOP':   0x0B,
      'FACTORY_RESET': 0xFF,
      'FW_UPDATE':    0xFE,
    };

    const payload = new Uint8Array(16);
    payload[0] = cmdMap[command] || 0x00;

    if (params.protocol) {
      payload[1] = this._protocolToByte(params.protocol);
    }
    if (params.uid) {
      const uidBytes = params.uid.replace(/:/g, '').match(/.{2}/g)?.map(b => parseInt(b, 16)) || [];
      payload[2] = uidBytes.length;
      uidBytes.forEach((b, i) => { payload[3 + i] = b; });
    }
    if (params.mode) {
      payload[1] = params.mode === 'card' ? 0x01 : 0x02;
    }

    return this._bytesToBase64(payload);
  }

  _protocolToByte(protocol) {
    const map = {
      'A_106': 0x01, 'A_212': 0x02, 'A_424': 0x03,
      'B_106': 0x04, 'F_212': 0x05, 'F_424': 0x06, 'V': 0x07,
    };
    return map[protocol] || 0x01;
  }

  _protocolFromByte(byte) {
    const map = {
      0x01: 'NFC-A', 0x02: 'NFC-A', 0x03: 'NFC-A',
      0x04: 'NFC-B', 0x05: 'NFC-F', 0x06: 'NFC-F', 0x07: 'NFC-V',
    };
    return map[byte] || 'Unknown';
  }

  _rfidProtocolFromByte(byte) {
    const map = {
      0x00: 'EM4100', 0x01: 'HID Prox II', 0x02: 'AWID', 0x03: 'ioProx',
    };
    return map[byte] || 'Unknown';
  }

  // ========== Utility ==========

  _base64ToBytes(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes;
  }

  _bytesToBase64(bytes) {
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }
}