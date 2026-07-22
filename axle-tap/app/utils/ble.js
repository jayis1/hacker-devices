// ble.js — BLE manager for AxleTap companion app
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import { BleManager } from 'react-native-ble-plx';
import {
  AXLETAP_SERVICE_UUID,
  AXLETAP_CMD_CHAR_UUID,
  AXLETAP_TELEM_CHAR_UUID,
  decodeFrame,
  MSG,
} from './protocol';

// A persistent manager. React Native requires a single BleManager instance.
let manager = null;
let device = null;
let telemSubscription = null;
let statusCallback = null;
let logCallback = null;

export function getManager() {
  if (!manager) manager = new BleManager();
  return manager;
}

// Scan for AxleTap devices (advertised service UUID = AXLETAP_SERVICE_UUID)
export function scanForDevices(onFound, onComplete) {
  const m = getManager();
  m.startDeviceScan([AXLETAP_SERVICE_UUID], null, (error, dev) => {
    if (error) {
      console.warn('BLE scan error', error);
      return;
    }
    if (dev) onFound(dev);
  });
  // Stop after 10 s
  setTimeout(() => {
    m.stopDeviceScan();
    if (onComplete) onComplete();
  }, 10000);
}

export async function connectToDevice(dev, onStatus, onLog) {
  statusCallback = onStatus;
  logCallback = onLog;
  try {
    const connected = await dev.connect();
    await connected.discoverAllServicesAndCharacteristics();
    device = connected;
    // Subscribe to telemetry
    telemSubscription = connected.monitorCharacteristicForService(
      AXLETAP_SERVICE_UUID,
      AXLETAP_TELEM_CHAR_UUID,
      (err, char) => {
        if (err || !char) return;
        const frame = decodeFrame(char.value, null);
        if (!frame) return;
        switch (frame.type) {
        case MSG.STATUS:
        case MSG.GPTP_STATE:
        case MSG.SVC_MAP:
          if (statusCallback) statusCallback(frame);
          break;
        case MSG.LOG:
          if (logCallback) logCallback(frame);
          break;
        default:
          break;
        }
      }
    );
    return true;
  } catch (e) {
    console.warn('Connect error', e);
    return false;
  }
}

export async function sendCommand(base64Frame) {
  if (!device) return false;
  try {
    await device.writeCharacteristicWithResponseForService(
      AXLETAP_SERVICE_UUID,
      AXLETAP_CMD_CHAR_UUID,
      base64Frame
    );
    return true;
  } catch (e) {
    console.warn('Write error', e);
    return false;
  }
}

export async function disconnect() {
  if (telemSubscription) { telemSubscription.remove(); telemSubscription = null; }
  if (device) { await device.cancelConnection(); device = null; }
}

export function isConnected() { return device != null; }