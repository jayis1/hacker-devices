// src/ble/BleManager.tsx — BLE connection manager for Chronos-Phantom
//
// Manages BLE GATT connection to the ESP32-C3 module on the device,
// providing a React context for sending commands and receiving events.
//
// Uses the Nordic UART Service convention (NUS):
//   Service UUID:  6e400001-b5a3-f393-e0a9-e50e24dcca9e
//   TX (app→dev):  6e400002-...
//   RX (dev→app):  6e400003-...
//
// Author: jayis1
// License: GPL-2.0

import React, { createContext, useContext, useState, useEffect, useRef, useCallback } from 'react';
import { BleManager, State as BleState } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';
import { encodeFrame, decodeFrame, parseStatus, BleFrame } from './protocol';
import { CMD, EVT, DeviceStatus } from '../types';

const NUS_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_TX_CHAR_UUID  = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX_CHAR_UUID  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

interface BleContextValue {
  connected: boolean;
  scanning: boolean;
  deviceName: string | null;
  status: DeviceStatus | null;
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  sendCommand: (opcode: number, payload: Uint8Array) => Promise<void>;
  onEvent: (callback: (frame: BleFrame) => void) => () => void;
  error: string | null;
}

const BleContext = createContext<BleContextValue | null>(null);

export function BleProvider({ children }: { children: React.ReactNode }) {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [deviceName, setDeviceName] = useState<string | null>(null);
  const [status, setStatus] = useState<DeviceStatus | null>(null);
  const [error, setError] = useState<string | null>(null);
  const bleRef = useRef<BleManager | null>(null);
  const deviceRef = useRef<any>(null);
  const rxBufferRef = useRef<Uint8Array>(new Uint8Array(0));
  const eventCallbacks = useRef<Set<(f: BleFrame) => void>>(new Set());
  const statusTimerRef = useRef<ReturnType<typeof setInterval> | null>(null);

  useEffect(() => {
    // Initialize BLE manager lazily (to avoid crashes in non-BLE environments)
    try {
      bleRef.current = new BleManager();
    } catch (e) {
      // Running on a platform without BLE (e.g., web dev) — that's fine
    }
    return () => {
      if (statusTimerRef.current) clearInterval(statusTimerRef.current);
      if (deviceRef.current) {
        deviceRef.current.cancelConnection().catch(() => {});
      }
    };
  }, []);

  const requestPermissions = async (): Promise<boolean> => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        (r) => r === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  };

  const handleRxData = useCallback((data: Uint8Array) => {
    // Append to buffer and try to decode complete frames
    const combined = new Uint8Array(rxBufferRef.current.length + data.length);
    combined.set(rxBufferRef.current);
    combined.set(data, rxBufferRef.current.length);

    let offset = 0;
    let decoded: BleFrame | null;
    while ((decoded = decodeFrame(combined.subarray(offset))) !== null) {
      const frame = decoded;
      // Dispatch event to callbacks
      eventCallbacks.current.forEach((cb) => cb(frame));

      // Handle status events internally
      if (frame.opcode === EVT.STATUS) {
        const st = parseStatus(frame.payload);
        if (st) setStatus(st);
      }
      offset += 3 + ((combined[offset + 1] << 8) | combined[offset + 2]);
    }
    rxBufferRef.current = combined.subarray(offset);
  }, []);

  const connect = useCallback(async () => {
    setError(null);
    if (!bleRef.current) {
      setError('BLE not available on this device');
      return;
    }
    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      setError('BLE permissions denied');
      return;
    }

    setScanning(true);
    try {
      const device = await new Promise<any>((resolve, reject) => {
        const subscription = bleRef.current!.onStateChange((state) => {
          if (state !== BleState.PoweredOn) {
            reject(new Error('BLE is not powered on'));
            subscription.remove();
            return;
          }
          bleRef.current!.startDeviceScan([NUS_SERVICE_UUID], null, (err, dev) => {
            if (err) {
              reject(err);
              subscription.remove();
              return;
            }
            if (dev && dev.name && dev.name.includes('Chronos')) {
              bleRef.current!.stopDeviceScan();
              subscription.remove();
              resolve(dev);
            }
          });
        });
      });

      await device.connect();
      await device.discoveryAllServicesAndCharacteristics();
      deviceRef.current = device;
      setDeviceName(device.name);
      setConnected(true);
      setScanning(false);

      // Subscribe to RX characteristic
      const rxChar = await device.readCharacteristicForService(NUS_SERVICE_UUID, NUS_RX_CHAR_UUID);
      device.monitorCharacteristicForService(NUS_SERVICE_UUID, NUS_RX_CHAR_UUID, (err, char) => {
        if (err) {
          setError(`RX error: ${err.message}`);
          return;
        }
        if (char?.value) {
          // Base64 decode
          const raw = atob(char.value);
          const bytes = new Uint8Array(raw.length);
          for (let i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);
          handleRxData(bytes);
        }
      });

      // Start periodic status polling (1 Hz)
      statusTimerRef.current = setInterval(() => {
        sendCommandInternal(CMD.GET_STATUS, new Uint8Array(0));
      }, 1000);
    } catch (e: any) {
      setError(`Connection failed: ${e.message}`);
      setScanning(false);
    }
  }, [handleRxData]);

  const sendCommandInternal = useCallback(async (opcode: number, payload: Uint8Array) => {
    if (!deviceRef.current) return;
    try {
      const frame = encodeFrame(opcode, payload);
      // Convert to base64 for react-native-ble-plx
      let binary = '';
      frame.forEach((b) => (binary += String.fromCharCode(b)));
      const base64 = btoa(binary);
      await deviceRef.current.writeCharacteristicWithoutResponseForService(
        NUS_SERVICE_UUID, NUS_TX_CHAR_UUID, base64
      );
    } catch (e: any) {
      setError(`Send failed: ${e.message}`);
    }
  }, []);

  const sendCommand = useCallback(async (opcode: number, payload: Uint8Array) => {
    if (!connected) {
      setError('Not connected to device');
      return;
    }
    await sendCommandInternal(opcode, payload);
  }, [connected, sendCommandInternal]);

  const disconnect = useCallback(async () => {
    if (statusTimerRef.current) {
      clearInterval(statusTimerRef.current);
      statusTimerRef.current = null;
    }
    if (deviceRef.current) {
      await deviceRef.current.cancelConnection();
      deviceRef.current = null;
    }
    setConnected(false);
    setDeviceName(null);
    setStatus(null);
    rxBufferRef.current = new Uint8Array(0);
  }, []);

  const onEvent = useCallback((callback: (f: BleFrame) => void) => {
    eventCallbacks.current.add(callback);
    return () => { eventCallbacks.current.delete(callback); };
  }, []);

  return (
    <BleContext.Provider value={{
      connected, scanning, deviceName, status,
      connect, disconnect, sendCommand, onEvent, error
    }}>
      {children}
    </BleContext.Provider>
  );
}

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) throw new Error('useBle must be used within BleProvider');
  return ctx;
}

// Base64 helpers (React Native may not have atob/btoa natively)
function atob(input: string): string {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let str = input.replace(/=+$/, '');
  let output = '';
  for (let bc = 0, bs = 0, buffer, i = 0;
       (buffer = str.charAt(i++));
       ~buffer && ((bs = bc % 4 ? bs * 64 + buffer : buffer), bc++ % 4)
        ? (output += String.fromCharCode(255 & (bs >> ((-2 * bc) & 6))))
        : 0) {
    buffer = chars.indexOf(buffer);
  }
  return output;
}

function btoa(input: string): string {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let str = input;
  let output = '';
  for (let bc = 0, bs = 0, buffer, i = 0;
       (buffer = str.charAt(i++));
       ~buffer && ((bs = bc % 4 ? bs * 64 + buffer : buffer), bc++ % 4)
        ? (output += chars.charAt(63 & (bs >> (6 - 2 * bc))))
        : 0) {
    buffer = chars.indexOf(buffer);
  }
  return output + (bc % 4 ? (chars.substr(63 & (bs << 2)) + '=='.substr(2 - (bc % 4))) : '');
}

// Author: jayis1
// License: GPL-2.0