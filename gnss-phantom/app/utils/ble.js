/*
 * utils/ble.js — BLE connection manager for GNSS-Phantom
 *
 * Manages scanning, connecting, and data exchange with the GNSS-Phantom
 * device via the Nordic UART Service (NUS) over BLE.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import { BleManager } from 'react-native-ble-plx';
import {
  NUS_SERVICE_UUID,
  NUS_TX_CHAR_UUID,
  NUS_RX_CHAR_UUID,
  FrameDecoder,
  parseStatus,
} from './protocol';

const DEVICE_NAME_PREFIX = 'GNSS-Phantom';

class BLEManager {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.decoder = new FrameDecoder();
    this.connected = false;
    this.statusCallback = null;
    this.responseCallback = null;
    this.scanCallback = null;
  }

  // Start scanning for GNSS-Phantom devices
  startScan(onDeviceFound) {
    this.scanCallback = onDeviceFound;
    this.manager.startDeviceScan(null, { allowDuplicates: false }, (error, device) => {
      if (error) {
        console.error('[BLE] Scan error:', error);
        return;
      }
      if (device.name && device.name.startsWith(DEVICE_NAME_PREFIX)) {
        this.scanCallback && this.scanCallback(device);
      }
    });
  }

  stopScan() {
    this.manager.stopDeviceScan();
  }

  // Connect to a device
  async connect(device) {
    try {
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();

      // Subscribe to RX notifications (device → app)
      this.device.setupNotifications(NUS_RX_CHAR_UUID, (data, error) => {
        if (error) {
          console.error('[BLE] Notification error:', error);
          return;
        }
        if (data) {
          const bytes = new Uint8Array(data.valueOf());
          const frames = this.decoder.feed(bytes);
          for (const frame of frames) {
            this.handleFrame(frame);
          }
        }
      });

      this.connected = true;
      return true;
    } catch (e) {
      console.error('[BLE] Connect failed:', e);
      return false;
    }
  }

  // Send a command (Uint8Array) to the device
  async send(bytes) {
    if (!this.connected || !this.device) {
      console.error('[BLE] Not connected');
      return false;
    }
    try {
      const base64 = this.bytesToBase64(bytes);
      await this.device.writeCharacteristicWithResponse(NUS_TX_CHAR_UUID, base64);
      return true;
    } catch (e) {
      console.error('[BLE] Write failed:', e);
      return false;
    }
  }

  // Handle a decoded frame from the device
  handleFrame(frame) {
    console.log('[BLE] Frame received, cmd=0x' + frame.cmd.toString(16));
    // Status response (cmd 0x82 = response to GET_STATUS)
    if (frame.cmd === 0x82 && frame.payload.length >= 1) {
      const status = parseStatus(frame.payload);
      if (status && this.statusCallback) {
        this.statusCallback(status);
      }
    } else if (this.responseCallback) {
      this.responseCallback(frame);
    }
  }

  // Register callbacks
  onStatus(cb) { this.statusCallback = cb; }
  onResponse(cb) { this.responseCallback = cb; }

  // Disconnect
  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.connected = false;
      this.device = null;
    }
  }

  // Poll status periodically
  startStatusPolling(intervalMs = 2000) {
    this.pollTimer = setInterval(async () => {
      if (this.connected) {
        const { cmdGetStatus } = require('./protocol');
        await this.send(cmdGetStatus());
      }
    }, intervalMs);
  }

  stopStatusPolling() {
    if (this.pollTimer) {
      clearInterval(this.pollTimer);
      this.pollTimer = null;
    }
  }

  // Convert Uint8Array to base64
  bytesToBase64(bytes) {
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }
}

// Singleton instance
export const bleManager = new BLEManager();