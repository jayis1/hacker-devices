/**
 * DeviceContext.js — BLE connection and device state management
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Provides a React context for managing the BLE connection to the
 * WattPhantom device, sending commands, and receiving responses /
 * streaming data during PD manipulation, covert channel, and
 * fingerprinting operations.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// WattPhantom BLE service and characteristic UUIDs (NUS-style transparent UART)
const WP_SERVICE_UUID      = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const WP_CMD_CHAR_UUID     = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Write
const WP_NOTIFY_CHAR_UUID  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Notify

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState('Disconnected');
  const [battery, setBattery] = useState(100);
  const [attackMode, setAttackMode] = useState(false);
  const [streamCallback, setStreamCallback] = useState(null);

  const manager = useRef(null);
  const cmdChar = useRef(null);
  const notifyChar = useRef(null);
  const responseBuffer = useRef(Buffer.alloc(0));
  const pendingResolver = useRef(null);

  if (!manager.current) {
    manager.current = new BleManager();
  }

  /**
   * Handle a complete line received from the device.
   * Author: jayis1
   */
  const handleLine = useCallback((line) => {
    // Route streaming data to the stream callback if set
    if (streamCallback.current &&
        (line.startsWith('COVERT_RX') || line.startsWith('PROG') ||
         line.startsWith('WARN') || line.startsWith('DONE'))) {
      streamCallback.current(line);
      return;
    }
    // Otherwise it's a command response — resolve the pending promise
    if (pendingResolver.current) {
      const resolve = pendingResolver.current;
      pendingResolver.current = null;
      resolve(line);
    }
  }, [streamCallback]);

  /**
   * Process incoming BLE notification data.
   * Author: jayis1
   */
  const handleNotification = useCallback((error, characteristic) => {
    if (error) {
      console.warn('BLE notification error:', error.message);
      return;
    }
    if (!characteristic?.value) return;

    const chunk = Buffer.from(characteristic.value, 'base64');
    responseBuffer.current = Buffer.concat([responseBuffer.current, chunk]);

    // Process complete lines (terminated by \r\n)
    let buf = responseBuffer.current;
    let newlineIdx;
    while ((newlineIdx = buf.indexOf('\n')) !== -1) {
      let line = buf.slice(0, newlineIdx).toString('utf8');
      if (line.endsWith('\r')) line = line.slice(0, -1);
      responseBuffer.current = buf.slice(newlineIdx + 1);
      buf = responseBuffer.current;
      if (line.length > 0) handleLine(line);
    }
  }, [handleLine]);

  /**
   * Scan for WattPhantom BLE devices.
   * Author: jayis1
   */
  const scanForDevices = useCallback(async () => {
    setStatus('Scanning...');
    return new Promise((resolve, reject) => {
      const found = [];
      const timeout = setTimeout(() => {
        manager.current.stopDeviceScan();
        setStatus('Scan complete');
        resolve(found);
      }, 10000);

      manager.current.startDeviceScan([WP_SERVICE_UUID], null, (error, dev) => {
        if (error) {
          clearTimeout(timeout);
          reject(error);
          return;
        }
        if (dev && !found.find((d) => d.id === dev.id)) {
          found.push(dev);
        }
      });
    });
  }, []);

  /**
   * Connect to a WattPhantom device by ID.
   * Author: jayis1
   */
  const connectToDevice = useCallback(async (deviceId) => {
    setStatus('Connecting...');
    try {
      const dev = await manager.current.connectToDevice(deviceId);
      await dev.discoverAllServicesAndCharacteristics();
      const chars = await dev.characteristicsForService(WP_SERVICE_UUID);

      cmdChar.current = chars.find((c) => c.uuid === WP_CMD_CHAR_UUID);
      notifyChar.current = chars.find((c) => c.uuid === WP_NOTIFY_CHAR_UUID);

      // Setup notification listener
      await notifyChar.current.monitor(handleNotification);

      setDevice(dev);
      setConnected(true);
      setStatus('Connected');
      return true;
    } catch (e) {
      setStatus('Connection failed: ' + e.message);
      return false;
    }
  }, [handleNotification]);

  /**
   * Disconnect from the device.
   * Author: jayis1
   */
  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
    }
    setDevice(null);
    setConnected(false);
    setStatus('Disconnected');
    setAttackMode(false);
  }, [device]);

  /**
   * Send a command to the device and wait for response.
   * Author: jayis1
   */
  const sendCommand = useCallback(async (cmd, timeoutMs = 3000) => {
    if (!cmdChar.current) {
      return 'ERR not connected';
    }

    return new Promise(async (resolve) => {
      // Set timeout
      const timer = setTimeout(() => {
        if (pendingResolver.current) {
          pendingResolver.current = null;
          resolve('ERR timeout');
        }
      }, timeoutMs);

      pendingResolver.current = (response) => {
        clearTimeout(timer);
        resolve(response);
      };

      try {
        const data = Buffer.from(cmd + '\r\n', 'utf8');
        await cmdChar.current.writeWithResponse(data.toString('base64'));
      } catch (e) {
        clearTimeout(timer);
        if (pendingResolver.current) {
          pendingResolver.current = null;
          resolve('ERR write failed: ' + e.message);
        }
      }
    });
  }, []);

  /**
   * Set a stream callback for continuous data (covert channel, power monitor).
   * Author: jayis1
   */
  const setStream = useCallback((callback) => {
    setStreamCallback({ current: callback });
  }, []);

  const value = {
    device,
    connected,
    status,
    battery,
    attackMode,
    setAttackMode,
    scanForDevices,
    connectToDevice,
    disconnect,
    sendCommand,
    setStream,
  };

  return (
    <DeviceContext.Provider value={value}>
      {children}
    </DeviceContext.Provider>
  );
}

/**
 * Hook to access the device context.
 * Author: jayis1
 */
export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) {
    throw new Error('useDevice must be used within DeviceProvider');
  }
  return ctx;
}