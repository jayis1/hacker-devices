/**
 * eddy-phantom — BLEManager.js
 * BLE connection management and protocol handling for
 * communication with the Eddy-Phantom device.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import { BleManager } from '@react-native-ble-plx/react-native-ble-plx';
import { Buffer } from 'base64-js';

// BLE UUIDs (shortened for readability — full UUIDs in App.js)
const SERVICE_UUID  = 'f0000001-0000-1000-8000-00805f9b34fb';
const CHAR_KEYSTREAM = 'f0000002-0000-1000-8000-00805f9b34fb';
const CHAR_COMMAND   = 'f0000003-0000-1000-8000-00805f9b34fb';
const CHAR_STATUS    = 'f0000004-0000-1000-8000-00805f9b34fb';
const CHAR_RAWBURST  = 'f0000005-0000-1000-8000-00805f9b34fb';
const CHAR_AUTH='f00000...34fb';

// Command opcodes
const CMD_ARM         = 0x01;
const CMD_DISARM      = 0x02;
const CMD_SET_PROFILE = 0x03;
const CMD_SET_THRESH  = 0x04;
const CMD_REQ_BURST   = 0x05;
const CMD_CAL_START   = 0x06;
const CMD_CAL_KEY     = 0x07;
const CMD_CAL_FINISH  = 0x08;
const CMD_STATUS      = 0x09;
const CMD_AUTH        = 0x0A;
const CMD_FW_UPDATE   = 0x0B;

class EddyBLEManager {
  constructor() {
    this.ble = new BleManager();
    this.device = null;
    this.connected = false;
    this.authenticated = false;

    // Callbacks set by App.js
    this.onConnectionChange = null;
    this.onStatusUpdate = null;
    this.onKeystroke = null;
    this.onRawBurst = null;

    // Pre-shared key (in production, securely stored)
    this.psk = new Uint8Array([
      0xED, 0xD0, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
      0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
      0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
    ]);
  }

  // ── Scan and connect to Eddy-Phantom device ───────────────────
  async scanAndConnect() {
    try {
      const device = await this.ble.connectToDevice(null, {
        autoConnect: true,
        requestMTU: 185,
      });

      if (device) {
        this.device = device;
        this.connected = true;
        if (this.onConnectionChange) this.onConnectionChange(true);

        // Discover services and characteristics
        await device.discoverAllServicesAndCharacteristics();
        await this.setupNotifications(device);

        // Authenticate
        await this.authenticate();

        return true;
      }
    } catch (error) {
      console.error('[Eddy-BLE] Scan/connect error:', error);
      // Fallback: scan by service UUID
      this.startScanning();
    }
    return false;
  }

  // ── Start scanning for the device ─────────────────────────────
  startScanning() {
    this.ble.startDeviceScan([SERVICE_UUID], null, (error, device) => {
      if (error) {
        console.error('[Eddy-BLE] Scan error:', error);
        return;
      }
      if (device && device.name === 'EDDY') {
        this.ble.stopDeviceScan();
        this.connectToDevice(device);
      }
    });
  }

  // ── Connect to a specific device ──────────────────────────────
  async connectToDevice(device) {
    try {
      const connected = await device.connect({ requestMTU: 185 });
      this.device = connected;
      this.connected = true;
      if (this.onConnectionChange) this.onConnectionChange(true);

      await connected.discoverAllServicesAndCharacteristics();
      await this.setupNotifications(connected);
      await this.authenticate();
    } catch (error) {
      console.error('[Eddy-BLE] Connect error:', error);
    }
  }

  // ── Set up BLE notifications ──────────────────────────────────
  async setupNotifications(device) {
    // Keystream notification (F002)
    await device.monitorCharacteristicForService(
      SERVICE_UUID, CHAR_KEYSTREAM,
      (error, characteristic) => {
        if (error) {
          console.error('[Eddy-BLE] Keystream notification error:', error);
          return;
        }
        if (characteristic && characteristic.value) {
          this.parseKeystreamData(characteristic.value);
        }
      }
    );

    // Status notification (F004)
    await device.monitorCharacteristicForService(
      SERVICE_UUID, CHAR_STATUS,
      (error, characteristic) => {
        if (error) {
          console.error('[Eddy-BLE] Status notification error:', error);
          return;
        }
        if (characteristic && characteristic.value) {
          this.parseStatusData(characteristic.value);
        }
      }
    );

    // Raw burst notification (F005) — for raw capture viewer
    await device.monitorCharacteristicForService(
      SERVICE_UUID, CHAR_RAWBURST,
      (error, characteristic) => {
        if (error) {
          console.error('[Eddy-BLE] Raw burst notification error:', error);
          return;
        }
        if (characteristic && characteristic.value) {
          this.parseRawBurstData(characteristic.value);
        }
      }
    );
  }

  // ── Parse keystream notification data ─────────────────────────
  parseKeystreamData(base64Data) {
    // Decode base64 to byte array
    const bytes = this.base64ToBytes(base64Data);
    if (bytes.length < 7) return;

    // Format: [0x01] [scancode] [confidence] [ts3] [ts2] [ts1] [ts0]
    if (bytes[0] !== 0x01) return;

    const scancode = bytes[1];
    const confidence = bytes[2];
    const timestamp = (bytes[3] << 24) | (bytes[4] << 16) |
                      (bytes[5] << 8) | bytes[6];

    if (this.onKeystroke) {
      this.onKeystroke(scancode, confidence, timestamp);
    }
  }

  // ── Parse status notification data ────────────────────────────
  parseStatusData(base64Data) {
    const bytes = this.base64ToBytes(base64Data);
    if (bytes.length < 4) return;

    // Format: [0x09] [state] [battery] [info_len] [info...]
    if (bytes[0] !== 0x09) return;

    const state = this.stateToString(bytes[1]);
    const battery = bytes[2];
    const infoLen = bytes[3];
    let info = '';
    for (let i = 0; i < infoLen && i + 4 < bytes.length; i++) {
      info += String.fromCharCode(bytes[4 + i]);
    }

    if (this.onStatusUpdate) {
      this.onStatusUpdate(state, battery, info);
    }
  }

  // ── Parse raw burst data (chunked) ────────────────────────────
  parseRawBurstData(base64Data) {
    const bytes = this.base64ToBytes(base64Data);
    // Raw burst data arrives in chunks. Accumulate and reconstruct.
    if (this.onRawBurst) {
      this.onRawBurst(bytes);
    }
  }

  // ── Authenticate with device ──────────────────────────────────
  async authenticate() {
    try {
      // Send PSK to auth characteristic
      const authData = this.bytesToBase64(this.psk);
      await this.device.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CHAR_AUTH, authData
      );
      this.authenticated = true;
      console.log('[Eddy-BLE] Authenticated successfully');
    } catch (error) {
      console.error('[Eddy-BLE] Auth error:', error);
    }
  }

  // ── Send arm command ──────────────────────────────────────────
  async arm() {
    await this.sendCommand(CMD_ARM);
  }

  // ── Send disarm command ───────────────────────────────────────
  async disarm() {
    await this.sendCommand(CMD_DISARM);
  }

  // ── Set active profile ────────────────────────────────────────
  async setProfile(profileName) {
    const payload = new Uint8Array(profileName.length);
    for (let i = 0; i < profileName.length; i++) {
      payload[i] = profileName.charCodeAt(i);
    }
    await this.sendCommand(CMD_SET_PROFILE, payload);
  }

  // ── Set confidence threshold ──────────────────────────────────
  async setThreshold(threshold) {
    const payload = new Uint8Array([threshold]);
    await this.sendCommand(CMD_SET_THRESH, payload);
  }

  // ── Request a raw burst by index ──────────────────────────────
  async requestBurst(burstIndex) {
    const payload = new Uint8Array(4);
    payload[0] = burstIndex & 0xFF;
    payload[1] = (burstIndex >> 8) & 0xFF;
    payload[2] = (burstIndex >> 16) & 0xFF;
    payload[3] = (burstIndex >> 24) & 0xFF;
    await this.sendCommand(CMD_REQ_BURST, payload);
  }

  // ── Start calibration mode ────────────────────────────────────
  async startCalibration(controllerId) {
    const payload = new Uint8Array(2);
    payload[0] = controllerId & 0xFF;
    payload[1] = (controllerId >> 8) & 0xFF;
    await this.sendCommand(CMD_CAL_START, payload);
  }

  // ── Add calibration key sample ────────────────────────────────
  async calibrateKey(scancode) {
    const payload = new Uint8Array([scancode]);
    await this.sendCommand(CMD_CAL_KEY, payload);
  }

  // ── Finish calibration ────────────────────────────────────────
  async finishCalibration() {
    await this.sendCommand(CMD_CAL_FINISH);
  }

  // ── Generic command sender ────────────────────────────────────
  async sendCommand(cmd, payload = null) {
    if (!this.device || !this.connected) {
      console.warn('[Eddy-BLE] Not connected');
      return;
    }

    let data;
    if (payload) {
      data = new Uint8Array(1 + payload.length);
      data[0] = cmd;
      data.set(payload, 1);
    } else {
      data = new Uint8Array([cmd]);
    }

    const base64 = this.bytesToBase64(data);
    try {
      await this.device.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CHAR_COMMAND, base64
      );
    } catch (error) {
      console.error('[Eddy-BLE] Send command error:', error);
    }
  }

  // ── Disconnect from device ────────────────────────────────────
  async disconnect() {
    if (this.device) {
      try {
        await this.device.cancelConnection();
      } catch (error) {
        console.error('[Eddy-BLE] Disconnect error:', error);
      }
    }
    this.device = null;
    this.connected = false;
    this.authenticated = false;
    if (this.onConnectionChange) this.onConnectionChange(false);
  }

  // ── Utility: state code to string ─────────────────────────────
  stateToString(state) {
    const states = ['BOOT', 'IDLE', 'ARMED', 'CAPTURE', 'CALIBRATE', 'ERROR'];
    return states[state] || 'UNKNOWN';
  }

  // ── Utility: base64 to byte array ─────────────────────────────
  base64ToBytes(base64) {
    const buffer = Buffer.from(base64, 'base64');
    return new Uint8Array(buffer);
  }

  // ── Utility: byte array to base64 ─────────────────────────────
  bytesToBase64(bytes) {
    return Buffer.from(bytes).toString('base64');
  }

  // ── Check Bluetooth state ─────────────────────────────────────
  async checkBluetoothState() {
    const state = await this.ble.state();
    return state === 'PoweredOn';
  }

  // ── Enable Bluetooth if disabled ──────────────────────────────
  async enableBluetooth() {
    const state = await this.ble.state();
    if (state !== 'PoweredOn') {
      // On Android, this may prompt the user
      this.ble.enable();
    }
  }
}

export default EddyBLEManager;