/**
 * BLEManager.js — BLE connection & protocol handler
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Wraps react-native-ble-plx to connect to the Harmonic Reaper wand,
 * subscribe to Nordic UART Service (NUS) notifications, and send
 * command frames. Exposes a simple event-driven API:
 *
 *   ble.onStatus = (statusObj) => {}
 *   ble.onHit = (hitObj) => {}
 *   ble.onConnected = (bool) => {}
 *   ble.sendCommand(op, payload)
 *   ble.disconnect()
 */

import { BleManager } from 'react-native-ble-plx';
import { decodeFrame, encodeFrame, OPCODES } from '../utils/protocol';

const NUS_SERVICE_UUID  = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHAR_UUID  = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';  // write
const NUS_TX_CHAR_UUID  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';  // notify

const DEVICE_NAME = 'HR-NLJD';
const SCAN_TIMEOUT = 10000;

export default class BLEManager {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.onStatus = null;
    this.onHit = null;
    this.onConnected = null;
    this._rxBuf = [];
    this._scan();
  }

  _scan = () => {
    this.manager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        console.warn('[BLE] scan error:', error.message);
        return;
      }
      if (device.name === DEVICE_NAME) {
        this.manager.stopDeviceScan();
        this._connect(device);
      }
    });
    setTimeout(() => {
      if (!this.device) this.manager.stopDeviceScan();
    }, SCAN_TIMEOUT);
  };

  _connect = async (device) => {
    try {
      await device.connect();
      await device.discoverAllServicesAndCharacteristics();
      this.device = device;

      // Subscribe to NUS TX notifications
      await device.monitorCharacteristicForService(
        NUS_SERVICE_UUID,
        NUS_TX_CHAR_UUID,
        (err, char) => {
          if (err || !char) return;
          const raw = char.value;  // base64
          this._handleNotification(base64ToBytes(raw));
        }
      );

      if (this.onConnected) this.onConnected(true);
    } catch (e) {
      console.warn('[BLE] connect failed:', e.message);
      // retry scan after delay
      setTimeout(this._scan, 3000);
    }
  };

  _handleNotification = (bytes) => {
    // Feed bytes into the frame parser
    for (const b of bytes) {
      this._rxBuf.push(b);
    }
    // Try to decode complete frames
    const frames = decodeFrame(this._rxBuf);
    for (const f of frames) {
      this._dispatchFrame(f);
    }
  };

  _dispatchFrame = (frame) => {
    switch (frame.op) {
      case OPCODES.NOTIF_STATUS:
        if (this.onStatus) {
          this.onStatus({
            mode: frame.payload[0],
            p2_dbfs: (frame.payload[1] | (frame.payload[2] << 8)) << 16 >> 16,
            p3_dbfs: (frame.payload[3] | (frame.payload[4] << 8)) << 16 >> 16,
            ratio_db: (frame.payload[5] << 24) >> 24,
            classify: frame.payload[6],
            tx_power_dbm: (frame.payload[7] << 24) >> 24,
            batt_pct: frame.payload[8],
            hit_count: frame.payload[9] | (frame.payload[10] << 8),
          });
        }
        break;
      case OPCODES.NOTIF_HIT: {
        const p = frame.payload;
        const hit = {
          timestamp_ms: p[0] | (p[1] << 8) | (p[2] << 16),
          pitch_deg: (p[3] | (p[4] << 8)) << 16 >> 16,
          yaw_deg: (p[5] | (p[6] << 8)) << 16 >> 16,
          p2_dbfs: (p[7] | (p[8] << 8)) << 16 >> 16,
          p3_dbfs: (p[9] | (p[10] << 8)) << 16 >> 16,
          ratio_db: (p[11] << 24) >> 24,
          classify: p[12],
          tx_power_dbm: (p[13] << 24) >> 24,
        };
        if (this.onHit) this.onHit(hit);
        break;
      }
      case OPCODES.NOTIF_EXPORT_DONE:
        console.log('[BLE] export done:', frame.payload[0] | (frame.payload[1] << 8));
        break;
      default:
        break;
    }
  };

  sendCommand = async (op, payload = []) => {
    if (!this.device) return;
    const frame = encodeFrame(op, payload);
    const base64 = bytesToBase64(frame);
    try {
      await this.device.writeCharacteristicWithResponseForService(
        NUS_SERVICE_UUID,
        NUS_RX_CHAR_UUID,
        base64
      );
    } catch (e) {
      console.warn('[BLE] write failed:', e.message);
    }
  };

  disconnect = () => {
    if (this.device) {
      this.device.cancelConnection();
      this.device = null;
    }
    if (this.onConnected) this.onConnected(false);
  };
}

// ---- base64 helpers ----
function base64ToBytes(b64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const lookup = {};
  for (let i = 0; i < chars.length; i++) lookup[chars[i]] = i;
  const clean = b64.replace(/=/g, '');
  const bytes = [];
  for (let i = 0; i < clean.length; i += 4) {
    const c0 = lookup[clean[i]] || 0;
    const c1 = lookup[clean[i + 1]] || 0;
    const c2 = lookup[clean[i + 2]] || 0;
    const c3 = lookup[clean[i + 3]] || 0;
    bytes.push((c0 << 2) | (c1 >> 4));
    if (i + 2 < clean.length) bytes.push(((c1 & 0xF) << 4) | (c2 >> 2));
    if (i + 3 < clean.length) bytes.push(((c2 & 0x3) << 6) | c3);
  }
  return bytes;
}

function bytesToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b0 = bytes[i] || 0;
    const b1 = bytes[i + 1] || 0;
    const b2 = bytes[i + 2] || 0;
    result += chars[b0 >> 2];
    result += chars[((b0 & 0x3) << 4) | (b1 >> 4)];
    result += (i + 1 < bytes.length) ? chars[((b1 & 0xF) << 2) | (b2 >> 6)] : '=';
    result += (i + 2 < bytes.length) ? chars[b2 & 0x3F] : '=';
  }
  return result;
}