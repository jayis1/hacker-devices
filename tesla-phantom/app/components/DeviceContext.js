/**
 * TeslaPhantom — DeviceContext
 * BLE device connection management and state shared across screens.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { createContext, useState, useContext, useEffect } from 'react';
import { BleManager } from 'react-native-ble-plx';

// GATT service UUID for TeslaPhantom
const SERVICE_UUID = '0000tes0-0000-1000-8000-00805f9b34fb';
const CMD_CHAR_UUID  = '0000tes1-0000-1000-8000-00805f9b34fb';
const STATUS_CHAR_UUID = '0000tes2-0000-1000-8000-00805f9b34fb';
const TRACE_CHAR_UUID = '0000tes3-0000-1000-8000-00805f9b34fb';
const SCANMAP_CHAR_UUID = '0000tes4-0000-1000-8000-00805f9b34fb';

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState({
    state: 0,
    battery: 100,
    mode: 'EMFI',
    armed: false,
    position: { x: 0, y: 0, z: 0 },
    hvVoltage: 0,
  });
  const [scanMap, setScanMap] = useState(null);
  const [traceData, setTraceData] = useState(null);
  const [scanning, setScanning] = useState(false);
  const [error, setError] = useState(null);

  // Scan for TeslaPhantom devices
  const startScan = () => {
    setScanning(true);
    setError(null);
    manager.startDeviceScan(null, null, (err, dev) => {
      if (err) {
        setError(err.message);
        setScanning(false);
        return;
      }
      if (dev.name === 'TeslaPhantom') {
        manager.stopDeviceScan();
        setScanning(false);
        connectToDevice(dev);
      }
    });
  };

  // Connect to a TeslaPhantom device
  const connectToDevice = async (dev) => {
    try {
      const connectedDev = await dev.connect();
      await connectedDev.discoverAllServicesAndCharacteristics();
      setDevice(connectedDev);
      setConnected(true);

      // Subscribe to status notifications
      connectedDev.setupNotification(STATUS_CHAR_UUID).subscribe(
        (char) => {
          if (char?.value) {
            const decoded = decodeBase64(char.value);
            parseStatus(decoded);
          }
        },
        (err) => setError(err.message)
      );

      // Subscribe to scan map notifications
      connectedDev.setupNotification(SCANMAP_CHAR_UUID).subscribe(
        (char) => {
          if (char?.value) {
            const decoded = decodeBase64(char.value);
            setScanMap(decoded);
          }
        },
        (err) => setError(err.message)
      );

      // Subscribe to trace data notifications
      connectedDev.setupNotification(TRACE_CHAR_UUID).subscribe(
        (char) => {
          if (char?.value) {
            const decoded = decodeBase64(char.value);
            setTraceData(decoded);
          }
        },
        (err) => setError(err.message)
      );
    } catch (err) {
      setError(err.message);
      setConnected(false);
    }
  };

  // Send a JSON command to the device
  const sendCommand = async (cmdObj) => {
    if (!device || !connected) {
      setError('Not connected to device');
      return;
    }
    try {
      const json = JSON.stringify(cmdObj);
      const encoded = encodeBase64(json);
      await device.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CMD_CHAR_UUID, encoded
      );
    } catch (err) {
      setError(err.message);
    }
  };

  // Parse status JSON from device
  const parseStatus = (jsonStr) => {
    try {
      const obj = JSON.parse(jsonStr);
      setStatus(prev => ({
        ...prev,
        state: obj.st ?? prev.state,
        battery: obj.bat ?? prev.battery,
        mode: obj.mode ?? prev.mode,
        armed: obj.st === 2 || obj.st === 4, // ARMED or PULSE
        position: obj.pos ?? prev.position,
        hvVoltage: obj.hv ?? prev.hvVoltage,
      }));
    } catch (e) {
      // Ignore parse errors
    }
  };

  // Disconnect
  const disconnect = async () => {
    if (device) {
      await device.cancelConnection();
      setConnected(false);
      setDevice(null);
    }
  };

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (manager) manager.destroy();
    };
  }, [manager]);

  const value = {
    manager,
    device,
    connected,
    status,
    scanMap,
    traceData,
    scanning,
    error,
    startScan,
    sendCommand,
    disconnect,
  };

  return (
    <DeviceContext.Provider value={value}>
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error('useDevice must be used within DeviceProvider');
  return ctx;
}

// Base64 helpers (React Native has btoa/atob, but this is portable)
function decodeBase64(str) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  let i = 0;
  while (i < str.length) {
    const a = chars.indexOf(str[i++] || '=');
    const b = chars.indexOf(str[i++] || '=');
    const c = chars.indexOf(str[i++] || '=');
    const d = chars.indexOf(str[i++] || '=');
    const n = (a << 18) | (b << 12) | (c << 6) | d;
    if (b >= 0) result += String.fromCharCode((n >> 16) & 0xFF);
    if (c >= 0) result += String.fromCharCode((n >> 8) & 0xFF);
    if (d >= 0) result += String.fromCharCode(n & 0xFF);
  }
  return result;
}

function encodeBase64(str) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  let i = 0;
  while (i < str.length) {
    const a = str.charCodeAt(i++);
    const b = i < str.length ? str.charCodeAt(i++) : -1;
    const c = i < str.length ? str.charCodeAt(i++) : -1;
    const n = (a << 16) | ((b & 0xFF) << 8) | (c & 0xFF);
    result += chars[(n >> 18) & 0x3F];
    result += chars[(n >> 12) & 0x3F];
    result += b >= 0 ? chars[(n >> 6) & 0x3F] : '=';
    result += c >= 0 ? chars[n & 0x3F] : '=';
  }
  return result;
}