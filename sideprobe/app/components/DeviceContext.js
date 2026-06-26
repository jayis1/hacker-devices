/**
 * DeviceContext.js — BLE connection and device state management
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Provides a React context for managing the BLE connection to the
 * SideProbe device, sending commands, and receiving responses / streaming
 * correlation results during a CPA attack.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// SideProbe BLE service and characteristic UUIDs (NUS-style transparent UART)
const SP_SERVICE_UUID      = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const SP_CMD_CHAR_UUID     = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Write
const SP_NOTIFY_CHAR_UUID  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Notify

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState('Disconnected');
  const [battery, setBattery] = useState(100);
  const [attackRunning, setAttackRunning] = useState(false);
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
    // Route streaming attack progress to the stream callback if set
    if (streamCallback.current && (line.startsWith('PROG') || line.startsWith('DONE')
        || line.startsWith('CONVERGED') || line.startsWith('STOPPED'))) {
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
   * Scan for SideProbe BLE devices.
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

      manager.current.startDeviceScan([SP_SERVICE_UUID], null, (error, dev) => {
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
   * Connect to a SideProbe device by ID.
   * Author: jayis1
   */
  const connectToDevice = useCallback(async (deviceId) => {
    setStatus('Connecting...');
    try {
      const dev = await manager.current.connectToDevice(deviceId);
      await dev.discoverAllServicesAndCharacteristics();
      const services = await dev.services();
      const chars = await dev.characteristicsForService(SP_SERVICE_UUID);

      cmdChar.current = chars.find((c) => c.uuid === SP_CMD_CHAR_UUID);
      notifyChar.current = chars.find((c) => c.uuid === SP_NOTIFY_CHAR_UUID);

      // Setup notification listener
      dev.monitorCharacteristicForService(
        SP_SERVICE_UUID,
        SP_NOTIFY_CHAR_UUID,
        (error, characteristic) => {
          if (error || !characteristic?.value) return;
          const buf = Buffer.from(characteristic.value, 'base64');
          responseBuffer.current = Buffer.concat([responseBuffer.current, buf]);
          // Split on newlines
          let idx;
          while ((idx = responseBuffer.current.indexOf(0x0A)) >= 0) {
            const line = responseBuffer.current.slice(0, idx).toString('utf8').trim();
            responseBuffer.current = responseBuffer.current.slice(idx + 1);
            if (line.length > 0) handleLine(line);
          }
        }
      );

      setDevice(dev);
      setConnected(true);
      setStatus('Connected');
      return true;
    } catch (e) {
      setStatus('Connection failed: ' + e.message);
      return false;
    }
  }, [handleLine]);

  /**
   * Send a command and await a single-line response (with timeout).
   * Author: jayis1
   */
  const sendCommand = useCallback(async (cmd, timeoutMs = 5000) => {
    if (!cmdChar.current) return null;
    const buf = Buffer.from(cmd + '\n', 'utf8');
    return new Promise((resolve) => {
      pendingResolver.current = resolve;
      cmdChar.current.writeWithoutResponse(buf.toString('base64'));
      setTimeout(() => {
        if (pendingResolver.current) {
          pendingResolver.current = null;
          resolve(null); // timeout
        }
      }, timeoutMs);
    });
  }, []);

  /**
   * Register a streaming callback for attack progress lines.
   */
  const setStreamHandler = useCallback((cb) => {
    streamCallback.current = cb;
  }, []);

  /**
   * Disconnect from the device.
   * Author: jayis1
   */
  const disconnect = useCallback(async () => {
    if (device) {
      await manager.current.cancelDeviceConnection(device.id);
    }
    setDevice(null);
    setConnected(false);
    setStatus('Disconnected');
    setAttackRunning(false);
  }, [device]);

  return (
    <DeviceContext.Provider
      value={{
        device,
        connected,
        status,
        battery,
        attackRunning,
        setAttackRunning,
        scanForDevices,
        connectToDevice,
        disconnect,
        sendCommand,
        setStreamHandler,
      }}
    >
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error('useDevice must be used within DeviceProvider');
  return ctx;
}