/**
 * DeviceContext.js — BLE connection and device state management
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Provides a React context for managing the BLE connection to
 * the WireReaper device, sending commands, and receiving responses.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// WireReaper BLE service and characteristic UUIDs
const WR_SERVICE_UUID       = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const WR_CMD_CHAR_UUID      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Write
const WR_NOTIFY_CHAR_UUID   = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Notify

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState('Disconnected');
  const [battery, setBattery] = useState(100);
  const [captureCount, setCaptureCount] = useState(0);
  const [isCapturing, setIsCapturing] = useState(false);
  const [streamCallback, setStreamCallback] = useState(null);

  const manager = useRef(null);
  const cmdChar = useRef(null);
  const notifyChar = useRef(null);
  const responseBuffer = useRef(Buffer.alloc(0));
  const pendingResolver = useRef(null);

  // Initialize BLE manager
  if (!manager.current) {
    manager.current = new BleManager();
  }

  /**
   * Scan for WireReaper BLE devices.
   * Author: jayis1
   */
  const scanForDevices = useCallback(async () => {
    setStatus('Scanning...');
    return new Promise((resolve, reject) => {
      const found = [];
      const timeout = setTimeout(() => {
        manager.current.stopDeviceScan();
        resolve(found);
      }, 10000);

      manager.current.startDeviceScan([WR_SERVICE_UUID], null, (error, dev) => {
        if (error) {
          clearTimeout(timeout);
          reject(error);
          return;
        }
        if (dev && dev.name && dev.name.includes('WireReaper')) {
          const existing = found.find(d => d.id === dev.id);
          if (!existing) {
            found.push({ id: dev.id, name: dev.name, rssi: dev.rssi });
          }
        }
      });
    });
  }, []);

  /**
   * Connect to a WireReaper device.
   * Author: jayis1
   */
  const connectToDevice = useCallback(async (deviceId) => {
    setStatus('Connecting...');
    try {
      const dev = await manager.current.devices[deviceId] ||
                   await manager.current.connectToDevice(deviceId);
      await dev.discoverAllServicesAndCharacteristics();
      const characteristics = await dev.characteristicsForService(WR_SERVICE_UUID);

      cmdChar.current = characteristics.find(c => c.uuid === WR_CMD_CHAR_UUID);
      notifyChar.current = characteristics.find(c => c.uuid === WR_NOTIFY_CHAR_UUID);

      // Subscribe to notifications for response data
      await notifyChar.current.monitor((error, char) => {
        if (error) return;
        if (char && char.value) {
          const data = Buffer.from(char.value, 'base64');
          handleResponseData(data);
        }
      });

      setDevice(dev);
      setConnected(true);
      setStatus('Connected to WireReaper');
      return true;
    } catch (error) {
      setStatus('Connection failed: ' + error.message);
      return false;
    }
  }, []);

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
    setIsCapturing(false);
  }, [device]);

  /**
   * Send a command to the WireReaper device.
   * Author: jayis1
   */
  const sendCommand = useCallback(async (opcode, payload = Buffer.alloc(0)) => {
    if (!cmdChar.current) {
      return null;
    }

    const frame = Buffer.concat([
      Buffer.from([opcode, payload.length]),
      payload,
    ]);

    return new Promise((resolve) => {
      pendingResolver.current = resolve;
      cmdChar.current.writeWithResponse(frame.toString('base64'))
        .catch(() => resolve(null));

      // Timeout after 5 seconds
      setTimeout(() => {
        if (pendingResolver.current === resolve) {
          pendingResolver.current = null;
          resolve(null);
        }
      }, 5000);
    });
  }, []);

  /**
   * Handle incoming response data from BLE notifications.
   * Author: jayis1
   */
  const handleResponseData = useCallback((data) => {
    // If we have a stream callback, feed it the data
    if (streamCallback && data.length > 2) {
      streamCallback(data);
      return;
    }

    // Accumulate into response buffer
    responseBuffer.current = Buffer.concat([responseBuffer.current, data]);

    // Check if we have a complete response (at least 2 bytes for opcode + status)
    if (responseBuffer.current.length >= 2 && pendingResolver.current) {
      const resolver = pendingResolver.current;
      pendingResolver.current = null;
      const response = responseBuffer.current;
      responseBuffer.current = Buffer.alloc(0);
      resolver(response);
    }

    // Update capture count if this is a status response
    if (responseBuffer.current.length >= 8 && responseBuffer.current[0] === 0x70) {
      const count = responseBuffer.current.readUInt32LE(4);
      setCaptureCount(count);
      setBattery(responseBuffer.current[17] || 100);
    }
  }, [streamCallback]);

  /**
   * Set a callback for streaming capture data.
   * Author: jayis1
   */
  const setStreamHandler = useCallback((callback) => {
    setStreamCallback(callback ? callback : null);
  }, []);

  const value = {
    device,
    connected,
    status,
    battery,
    captureCount,
    isCapturing,
    setIsCapturing,
    scanForDevices,
    connectToDevice,
    disconnect,
    sendCommand,
    setStreamHandler,
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