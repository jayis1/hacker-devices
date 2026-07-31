/**
 * BLEContext.js — BLE connection management for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Manages the BLE connection to the MagLance device's nRF52840 module.
 * Provides connection state, command sending, and telemetry parsing.
 */

import React, { createContext, useContext, useState, useEffect, useRef } from 'react';
import { BleManager } from 'react-native-ble-plx';
import base64 from 'react-native-base64';

// BLE service and characteristic UUIDs for MagLance
const MAGLANCE_SERVICE_UUID = '0000a1b2-0000-1000-8000-00805f9b34fb';
const MAGLANCE_TX_CHAR_UUID  = '0000a1b3-0000-1000-8000-00805f9b34fb'; // Device → Phone
const MAGLANCE_RX_CHAR_UUID  = '0000a1b4-0000-1000-8000-00805f9b34fb'; // Phone → Device

const BLEContext = createContext(null);

export const BLEProvider = ({ children }) => {
  const [device, setDevice] = useState(null);
  const [connectionState, setConnectionState] = useState('disconnected');
  const [telemetry, setTelemetry] = useState({
    mode: 'IDLE',
    current: 0,
    vbat: 3700,
    tempBridge: 25,
    tempCoil: 25,
    fieldX: [0, 0, 0],
    fieldY: [0, 0, 0],
    fieldZ: [0, 0, 0],
    profile: 0,
    tipId: 0,
    error: null,
  });
  const [scanResults, setScanResults] = useState([]);
  const manager = useRef(null);
  const responseBuffer = useRef('');

  useEffect(() => {
    manager.current = new BleManager();
    return () => {
      if (manager.current) {
        manager.current.destroy();
      }
    };
  }, []);

  /**
   * Scan for MagLance BLE devices
   */
  const scanForDevices = async () => {
    setConnectionState('scanning');
    setScanResults([]);

    manager.current.startDeviceScan(null, null, (error, scannedDevice) => {
      if (error) {
        console.error('Scan error:', error);
        setConnectionState('error');
        return;
      }

      if (scannedDevice && scannedDevice.name &&
          scannedDevice.name.includes('MagLance')) {
        setScanResults(prev => {
          const exists = prev.find(d => d.id === scannedDevice.id);
          if (exists) return prev;
          return [...prev, scannedDevice];
        });
      }
    });

    // Stop scan after 5 seconds
    setTimeout(() => {
      manager.current.stopDeviceScan();
      setConnectionState('scan_complete');
    }, 5000);
  };

  /**
   * Connect to a specific device
   */
  const connectToDevice = async (deviceId) => {
    setConnectionState('connecting');

    try {
      const connectedDevice = await manager.current.connectToDevice(deviceId);
      await connectedDevice.discoverAllServicesAndCharacteristics();

      // Set up notification listener for telemetry
      await manager.current.monitorCharacteristicForDevice(
        connectedDevice.id,
        MAGLANCE_SERVICE_UUID,
        MAGLANCE_TX_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            console.error('Notification error:', error);
            return;
          }
          if (characteristic && characteristic.value) {
            const data = base64.decode(characteristic.value);
            handleResponse(data);
          }
        }
      );

      setDevice(connectedDevice);
      setConnectionState('connected');

      // Request initial status
      await sendCommand('GET STATUS\n');
    } catch (error) {
      console.error('Connection error:', error);
      setConnectionState('error');
    }
  };

  /**
   * Disconnect from device
   */
  const disconnect = async () => {
    if (device) {
      await device.cancelConnection();
      setDevice(null);
      setConnectionState('disconnected');
    }
  };

  /**
   * Send a command to the device
   */
  const sendCommand = async (command) => {
    if (!device || connectionState !== 'connected') {
      console.warn('Not connected');
      return;
    }

    try {
      const encoded = base64.encode(command);
      await device.writeCharacteristicWithResponseForService(
        MAGLANCE_SERVICE_UUID,
        MAGLANCE_RX_CHAR_UUID,
        encoded
      );
    } catch (error) {
      console.error('Send error:', error);
    }
  };

  /**
   * Handle response data from the device
   */
  const handleResponse = (data) => {
    responseBuffer.current += data;

    // Process complete lines
    while (responseBuffer.current.includes('\n')) {
      const lineEnd = responseBuffer.current.indexOf('\n');
      const line = responseBuffer.current.substring(0, lineEnd).trim();
      responseBuffer.current = responseBuffer.current.substring(lineEnd + 1);

      if (line.startsWith('OK')) {
        parseTelemetry(line.substring(3));
      } else if (line.startsWith('ERR')) {
        const parts = line.split(' ');
        setTelemetry(prev => ({
          ...prev,
          error: parts[2] || 'Unknown error',
        }));
      }
    }
  };

  /**
   * Parse telemetry string from GET STATUS response
   * Format: "OK <mode> <current> <vbat> <temp1> <temp2> <profile>"
   */
  const parseTelemetry = (data) => {
    const parts = data.split(' ');
    if (parts.length >= 6) {
      setTelemetry(prev => ({
        ...prev,
        mode: parts[0] || 'IDLE',
        current: parseInt(parts[1]) || 0,
        vbat: parseInt(parts[2]) || 0,
        tempBridge: parseInt(parts[3]) || 25,
        tempCoil: parseInt(parts[4]) || 25,
        profile: parseInt(parts[5]) || 0,
        error: null,
      }));
    }
  };

  /**
   * Update field readings from GET FIELD response
   * Format: "OK <x0> <y0> <z0> <x1> <y1> <z1> <x2> <y2> <z2>"
   */
  const updateFieldReadings = (data) => {
    const parts = data.split(' ');
    if (parts.length >= 9) {
      setTelemetry(prev => ({
        ...prev,
        fieldX: [parseInt(parts[0]), parseInt(parts[3]), parseInt(parts[6])],
        fieldY: [parseInt(parts[1]), parseInt(parts[4]), parseInt(parts[7])],
        fieldZ: [parseInt(parts[2]), parseInt(parts[5]), parseInt(parts[8])],
      }));
    }
  };

  // Poll status every 500ms when connected
  useEffect(() => {
    if (connectionState !== 'connected') return;
    const interval = setInterval(() => {
      sendCommand('GET STATUS\n');
      sendCommand('GET FIELD\n');
    }, 500);
    return () => clearInterval(interval);
  }, [connectionState]);

  const value = {
    device,
    connectionState,
    telemetry,
    scanResults,
    scanForDevices,
    connectToDevice,
    disconnect,
    sendCommand,
  };

  return <BLEContext.Provider value={value}>{children}</BLEContext.Provider>;
};

export const useBLE = () => {
  const context = useContext(BLEContext);
  if (!context) {
    throw new Error('useBLE must be used within BLEProvider');
  }
  return context;
};