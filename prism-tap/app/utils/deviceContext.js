/**
 * utils/deviceContext.js — BLE Device Context for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Provides a React Context that manages the BLE connection to the Prism-Tap
 * device, including scanning, connecting, command sending, and state updates.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager, State } from 'react-native-ble-plx';
import base64 from 'react-native-base64';

// BLE service and characteristic UUIDs for Prism-Tap
const PRISM_TAP_SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb';
const PRISM_TAP_CMD_CHAR_UUID = '0000ffe1-0000-1000-8000-00805f9b34fb';
const PRISM_TAP_DATA_CHAR_UUID = '0000ffe2-0000-1000-8000-00805f9b34fb';

// Device status structure
const initialStatus = {
  connected: false,
  connecting: false,
  device: null,
  firmwareVersion: '---',
  mode: 'IDLE',
  batteryPct: 0,
  batteryMv: 0,
  sdPresent: false,
  bleConnected: false,
  usbConnected: false,
  fpgaReady: false,
  csiLinkUp: false,
  dsiLinkUp: false,
  uptime: 0,
  framesCaptured: 0,
  bytesWritten: 0,
  injectActive: false,
  framesInjected: 0,
  error: null,
};

const DeviceContext = createContext({
  ...initialStatus,
  scan: () => {},
  connect: () => {},
  disconnect: () => {},
  sendCommand: () => {},
  refreshStatus: () => {},
  discoveredDevices: [],
});

export const DeviceProvider = ({ children }) => {
  const [status, setStatus] = useState(initialStatus);
  const [discoveredDevices, setDiscoveredDevices] = useState([]);
  const bleManager = useRef(null);
  const connectedDevice = useRef(null);
  const commandChar = useRef(null);
  const dataChar = useRef(null);

  // Initialize BLE manager
  const getBleManager = () => {
    if (!bleManager.current) {
      bleManager.current = new BleManager();
    }
    return bleManager.current;
  };

  // Scan for Prism-Tap devices
  const scan = useCallback(() => {
    const manager = getBleManager();
    setStatus(prev => ({ ...prev, connecting: true, error: null }));

    manager.startDeviceScan([PRISM_TAP_SERVICE_UUID], null, (error, device) => {
      if (error) {
        setStatus(prev => ({ ...prev, connecting: false, error: error.message }));
        return;
      }

      if (device && device.name && device.name.includes('Prism-Tap')) {
        setDiscoveredDevices(prev => {
          const exists = prev.find(d => d.id === device.id);
          if (exists) return prev;
          return [...prev, device];
        });
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      manager.stopDeviceScan();
      setStatus(prev => ({ ...prev, connecting: false }));
    }, 10000);
  }, []);

  // Connect to a device
  const connect = useCallback(async (deviceId) => {
    const manager = getBleManager();
    setStatus(prev => ({ ...prev, connecting: true, error: null }));

    try {
      const device = await manager.connectToDevice(deviceId);
      await device.discoverAllServicesAndCharacteristics();

      const characteristics = await device.characteristicsForService(PRISM_TAP_SERVICE_UUID);
      const cmdChar = characteristics.find(c => c.uuid === PRISM_TAP_CMD_CHAR_UUID);
      const dChar = characteristics.find(c => c.uuid === PRISM_TAP_DATA_CHAR_UUID);

      connectedDevice.current = device;
      commandChar.current = cmdChar;
      dataChar.current = dChar;

      // Setup notification listener for data characteristic
      if (dChar) {
        await device.setupNotifications(dChar);
        device.monitorCharacteristicForService(
          PRISM_TAP_SERVICE_UUID,
          PRISM_TAP_DATA_CHAR_UUID,
          (error, characteristic) => {
            if (error) {
              console.warn('Notification error:', error.message);
              return;
            }
            if (characteristic && characteristic.value) {
              // Handle incoming data (frame thumbnails, status updates)
              const data = base64.decode(characteristic.value);
              // Process incoming data...
            }
          }
        );
      }

      setStatus(prev => ({
        ...prev,
        connected: true,
        connecting: false,
        device: device,
        error: null,
      }));

      // Immediately refresh status after connecting
      setTimeout(() => refreshStatus(), 500);
    } catch (error) {
      setStatus(prev => ({
        ...prev,
        connecting: false,
        error: error.message,
      }));
    }
  }, []);

  // Disconnect from device
  const disconnect = useCallback(async () => {
    if (connectedDevice.current) {
      await connectedDevice.current.cancelConnection();
      connectedDevice.current = null;
      commandChar.current = null;
      dataChar.current = null;
    }
    setStatus(initialStatus);
  }, []);

  // Send a command to the device
  const sendCommand = useCallback(async (opcode, payload = []) => {
    if (!commandChar.current || !connectedDevice.current) {
      setStatus(prev => ({ ...prev, error: 'Not connected' }));
      return null;
    }

    // Build command packet: opcode + seq + length(2) + crc(2) + payload
    const seq = Math.floor(Math.random() * 256);
    const length = payload.length;
    const crc = computeCRC16([opcode, seq, length & 0xFF, (length >> 8) & 0xFF, ...payload]);

    const packet = [
      opcode,
      seq,
      length & 0xFF,
      (length >> 8) & 0xFF,
      crc & 0xFF,
      (crc >> 8) & 0xFF,
      ...payload,
    ];

    const base64Data = base64.fromCharCode(...packet);

    try {
      await commandChar.current.writeWithResponse(base64Data);
      // In a real implementation, we'd wait for the response notification
      return true;
    } catch (error) {
      setStatus(prev => ({ ...prev, error: error.message }));
      return null;
    }
  }, []);

  // Refresh device status
  const refreshStatus = useCallback(async () => {
    if (!connectedDevice.current) return;

    const result = await sendCommand(0x02); // CMD_GET_STATUS
    if (result) {
      // Parse status response (would come from notification)
      // For now, just update the connected state
      setStatus(prev => ({
        ...prev,
        bleConnected: true,
      }));
    }
  }, [sendCommand]);

  // CRC-16-CCITT computation
  const computeCRC16 = (data) => {
    let crc = 0xFFFF;
    for (const byte of data) {
      crc ^= (byte & 0xFF) << 8;
      for (let i = 0; i < 8; i++) {
        if (crc & 0x8000) {
          crc = (crc << 1) ^ 0x1021;
        } else {
          crc <<= 1;
        }
      }
    }
    return crc & 0xFFFF;
  };

  const value = {
    ...status,
    scan,
    connect,
    disconnect,
    sendCommand,
    refreshStatus,
    discoveredDevices,
  };

  return (
    <DeviceContext.Provider value={value}>
      {children}
    </DeviceContext.Provider>
  );
};

export const useDevice = () => {
  const context = useContext(DeviceContext);
  if (!context) {
    throw new Error('useDevice must be used within a DeviceProvider');
  }
  return context;
};

export default DeviceContext;