/**
 * BLEManager.js — BLE Connection Manager for ECHO-Phantom
 *
 * Author: jayis1
 * License: MIT
 *
 * Manages the BLE 5.0 connection to the ECHO-Phantom device.
 * Provides a React context with connection state, command sending,
 * and audio stream reception.
 *
 * BLE GATT Service: 0000ec00-0000-1000-8000-00805f9b34fb
 *   Characteristic 0x0001 (Write):     Command input
 *   Characteristic 0x0002 (Notify):    Response/telemetry output
 *   Characteristic 0x0003 (Notify):   Audio stream output
 */

import React, { createContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';

// BLE UUIDs
const SERVICE_UUID = '0000ec00-0000-1000-8000-00805f9b34fb';
const CMD_CHAR_UUID = '0000ec01-0000-1000-8000-00805f9b34fb';
const RESP_CHAR_UUID = '0000ec02-0000-1000-8000-00805f9b34fb';
const AUDIO_CHAR_UUID = '0000ec03-0000-1000-8000-00805f9b34fb';

// Command codes (must match firmware)
const CMD_PING = 0x01;
const CMD_GET_STATUS = 0x02;
const CMD_GET_FORMAT = 0x03;
const CMD_START_CAPTURE = 0x04;
const CMD_STOP_CAPTURE = 0x05;
const CMD_START_INJECT = 0x06;
const CMD_STOP_INJECT = 0x07;
const CMD_UPLOAD_CLIP = 0x08;
const CMD_SET_MODE = 0x09;
const CMD_SET_FILTER = 0x0A;
const CMD_STREAM_START = 0x0B;
const CMD_STREAM_STOP = 0x0C;
const CMD_ERASE_CLIPS = 0x0D;
const CMD_GET_BATTERY = 0x0E;
const CMD_FACTORY_RESET = 0x0F;

// CRC-8 (Maxim/Dallas polynomial, matches firmware)
function crc8(data) {
  let crc = 0x00;
  const poly = 0x31;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ poly) & 0xFF;
      } else {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  return crc;
}

// Build a command packet with CRC
function buildCommand(cmdCode, payload = []) {
  const packet = [cmdCode, payload.length, ...payload];
  const crc = crc8(packet);
  packet.push(crc);
  return new Uint8Array(packet);
}

// Parse status response from device
function parseStatusResponse(data) {
  if (data.length < 40) return null;
  return {
    uptime: data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24),
    mode: data[6],
    sampleRate: data[7] | (data[8] << 8) | (data[9] << 16) | (data[10] << 24),
    bitDepth: data[11],
    channels: data[12],
    protocol: data[13],
    captureFrames: data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24),
    captureBytes: data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24),
    bufferFill: data[24] | (data[25] << 8),
    batteryMv: data[28] | (data[29] << 8),
    batteryPct: data[30],
    charging: !!(data[31] & 0x01),
    sdPresent: !!(data[31] & 0x02),
    bleConnected: !!(data[31] & 0x04),
    usbConnected: !!(data[31] & 0x08),
  };
}

export const BLEContext = createContext(null);

export function BLEProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [status, setStatus] = useState(null);
  const [error, setError] = useState(null);
  const [audioData, setAudioData] = useState(null);

  const managerRef = useRef(null);
  const deviceRef = useRef(null);
  const audioCallbackRef = useRef(null);

  // Request BLE permissions (Android)
  const requestPermissions = useCallback(async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      ]);
      return Object.values(granted).every(
        v => v === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  }, []);

  // Initialize BLE manager
  const initManager = useCallback(() => {
    if (!managerRef.current) {
      managerRef.current = new BleManager();
    }
    return managerRef.current;
  }, []);

  // Scan for ECHO-Phantom devices
  const scanAndConnect = useCallback(async () => {
    try {
      setError(null);
      const hasPermission = await requestPermissions();
      if (!hasPermission) {
        setError('BLE permissions not granted');
        return;
      }

      const manager = initManager();
      setScanning(true);

      manager.startDeviceScan(null, null, (err, device) => {
        if (err) {
          setError(`Scan error: ${err.message}`);
          setScanning(false);
          return;
        }

        // Look for devices advertising our service UUID
        if (device.name && device.name.includes('ECHO-Phantom')) {
          manager.stopDeviceScan();
          setScanning(false);

          device.connect()
            .then(d => d.discoverAllServicesAndCharacteristics())
            .then(d => {
              deviceRef.current = d;
              setConnected(true);

              // Subscribe to response notifications
              d.monitorCharacteristicForService(
                SERVICE_UUID, RESP_CHAR_UUID,
                (err, char) => {
                  if (err || !char || !char.value) return;
                  const data = base64ToBytes(char.value);
                  // Parse response and update status
                  if (data[0] === 0x01 && data.length >= 40) {
                    setStatus(parseStatusResponse(data));
                  }
                }
              );

              // Subscribe to audio stream notifications
              d.monitorCharacteristicForService(
                SERVICE_UUID, AUDIO_CHAR_UUID,
                (err, char) => {
                  if (err || !char || !char.value) return;
                  const data = base64ToBytes(char.value);
                  setAudioData(data);
                  if (audioCallbackRef.current) {
                    audioCallbackRef.current(data);
                  }
                }
              );
            })
            .catch(err => {
              setError(`Connection failed: ${err.message}`);
              setConnected(false);
            });
        }
      });
    } catch (err) {
      setError(`Init error: ${err.message}`);
    }
  }, [initManager, requestPermissions]);

  // Send a command to the device
  const sendCommand = useCallback(async (cmdCode, payload = []) => {
    if (!deviceRef.current || !connected) {
      setError('Device not connected');
      return null;
    }

    try {
      const packet = buildCommand(cmdCode, payload);
      const base64 = bytesToBase64(packet);

      await deviceRef.current.writeCharacteristicWithResponseForService(
        SERVICE_UUID,
        CMD_CHAR_UUID,
        base64
      );
      return true;
    } catch (err) {
      setError(`Send failed: ${err.message}`);
      return null;
    }
  }, [connected]);

  // Get current status from device
  const getStatus = useCallback(async () => {
    return sendCommand(CMD_GET_STATUS);
  }, [sendCommand]);

  // Start audio capture
  const startCapture = useCallback(async (destination) => {
    return sendCommand(CMD_START_CAPTURE, [destination || 0]);
  }, [sendCommand]);

  // Stop audio capture
  const stopCapture = useCallback(async () => {
    return sendCommand(CMD_STOP_CAPTURE);
  }, [sendCommand]);

  // Start audio injection
  const startInjection = useCallback(async (clipId, mode, gain) => {
    return sendCommand(CMD_START_INJECT, [clipId, mode, gain || 100]);
  }, [sendCommand]);

  // Stop audio injection
  const stopInjection = useCallback(async () => {
    return sendCommand(CMD_STOP_INJECT);
  }, [sendCommand]);

  // Upload an inject clip
  const uploadClip = useCallback(async (clipId, data, totalSize) => {
    const CHUNK_SIZE = 200; // BLE MTU limited
    for (let offset = 0; offset < data.length; offset += CHUNK_SIZE) {
      const chunk = data.slice(offset, offset + CHUNK_SIZE);
      const payload = [
        clipId,
        totalSize & 0xFF, (totalSize >> 8) & 0xFF,
        (totalSize >> 16) & 0xFF, (totalSize >> 24) & 0xFF,
        offset & 0xFF, (offset >> 8) & 0xFF,
        (offset >> 16) & 0xFF, (offset >> 24) & 0xFF,
        chunk.length & 0xFF, (chunk.length >> 8) & 0xFF,
        ...chunk
      ];
      await sendCommand(CMD_UPLOAD_CLIP, payload);
    }
    return true;
  }, [sendCommand]);

  // Set operating mode
  const setMode = useCallback(async (mode) => {
    return sendCommand(CMD_SET_MODE, [mode]);
  }, [sendCommand]);

  // Set DSP filter
  const setFilter = useCallback(async (filterType, coeffs) => {
    const payload = [filterType, coeffs.length];
    for (const c of coeffs) {
      payload.push(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF);
    }
    return sendCommand(CMD_SET_FILTER, payload);
  }, [sendCommand]);

  // Start audio streaming
  const startStream = useCallback(async (quality) => {
    return sendCommand(CMD_STREAM_START, [quality || 16]);
  }, [sendCommand]);

  // Stop audio streaming
  const stopStream = useCallback(async () => {
    return sendCommand(CMD_STREAM_STOP);
  }, [sendCommand]);

  // Erase all inject clips
  const eraseClips = useCallback(async () => {
    return sendCommand(CMD_ERASE_CLIPS);
  }, [sendCommand]);

  // Get battery status
  const getBattery = useCallback(async () => {
    return sendCommand(CMD_GET_BATTERY);
  }, [sendCommand]);

  // Factory reset
  const factoryReset = useCallback(async () => {
    return sendCommand(CMD_FACTORY_RESET, [0xDE, 0xAD, 0xBE, 0xEF]);
  }, [sendCommand]);

  // Register audio callback
  const setAudioCallback = useCallback((callback) => {
    audioCallbackRef.current = callback;
  }, []);

  // Disconnect
  const disconnect = useCallback(async () => {
    if (deviceRef.current) {
      await deviceRef.current.cancelConnection();
      deviceRef.current = null;
      setConnected(false);
      setStatus(null);
    }
  }, []);

  // Cleanup on unmount
  React.useEffect(() => {
    return () => {
      if (managerRef.current) {
        managerRef.current.destroy();
      }
    };
  }, []);

  const value = {
    connected,
    scanning,
    status,
    error,
    audioData,
    scanAndConnect,
    disconnect,
    sendCommand,
    getStatus,
    startCapture,
    stopCapture,
    startInjection,
    stopInjection,
    uploadClip,
    setMode,
    setFilter,
    startStream,
    stopStream,
    eraseClips,
    getBattery,
    factoryReset,
    setAudioCallback,
  };

  return <BLEContext.Provider value={value}>{children}</BLEContext.Provider>;
}

// Helper: Base64 to byte array
function base64ToBytes(base64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const lookup = {};
  for (let i = 0; i < chars.length; i++) lookup[chars[i]] = i;

  const clean = base64.replace(/=+$/, '');
  const bytes = [];
  for (let i = 0; i < clean.length; i += 4) {
    const enc1 = lookup[clean[i]] || 0;
    const enc2 = lookup[clean[i+1]] || 0;
    const enc3 = lookup[clean[i+2]] || 0;
    const enc4 = lookup[clean[i+3]] || 0;

    bytes.push((enc1 << 2) | (enc2 >> 4));
    if (clean[i+2] !== undefined) bytes.push(((enc2 & 15) << 4) | (enc3 >> 2));
    if (clean[i+3] !== undefined) bytes.push(((enc3 & 3) << 6) | enc4);
  }
  return bytes;
}

// Helper: Byte array to Base64
function bytesToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b1 = bytes[i] || 0;
    const b2 = bytes[i+1] || 0;
    const b3 = bytes[i+2] || 0;

    result += chars[b1 >> 2];
    result += chars[((b1 & 3) << 4) | (b2 >> 4)];
    result += (i + 1 < bytes.length) ? chars[((b2 & 15) << 2) | (b3 >> 6)] : '=';
    result += (i + 2 < bytes.length) ? chars[b3 & 63] : '=';
  }
  return result;
}