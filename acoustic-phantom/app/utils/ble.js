/**
 * ACOUSTIC-PHANTOM — BLE Manager
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * BLE connection manager using react-native-ble-plx. Provides a React
 * context for connection state, device status, event stream, and
 * command dispatch. Handles GATT characteristic discovery, notification
 * subscription, and the binary protocol codec (delegated to protocol.js).
 */

import React, {
  createContext,
  useContext,
  useState,
  useRef,
  useCallback,
  useEffect,
} from 'react';
import { BleManager } from 'react-native-ble-plx';
import {
  SERVICE_UUID,
  CMD_CHARACTERISTIC_UUID,
  EVENT_CHARACTERISTIC_UUID,
  STATUS_CHARACTERISTIC_UUID,
  AUDIO_CHARACTERISTIC_UUID,
  CALIB_CHARACTERISTIC_UUID,
  FILE_CHARACTERISTIC_UUID,
  decodeStatus,
  decodeEvent,
  decodeAudio,
  decodeFileList,
  encodeCommand,
} from './protocol';

const BLEContext = createContext(null);

export function useBLE() {
  return useContext(BLEContext);
}

export function BLEProvider({ children }) {
  const manager = useRef(null);
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [status, setStatus] = useState(null);
  const [events, setEvents] = useState([]);
  const [audioData, setAudioData] = useState(null);
  const [fileList, setFileList] = useState([]);
  const [calibProgress, setCalibProgress] = useState(null);
  const [error, setError] = useState(null);

  // Initialize BLE manager on mount
  useEffect(() => {
    manager.current = new BleManager();
    return () => {
      if (manager.current) {
        manager.current.destroy();
      }
    };
  }, []);

  // Scan for ACOUSTIC-PHANTOM devices
  const scan = useCallback(() => {
    if (!manager.current || scanning) return;
    setScanning(true);
    setError(null);

    manager.current.startDeviceScan([SERVICE_UUID], null, (err, dev) => {
      if (err) {
        setError(err.message);
        setScanning(false);
        return;
      }
      if (dev && dev.name && dev.name.includes('ACOUSTIC-PHANTOM')) {
        manager.current.stopDeviceScan();
        setScanning(false);
        setDevice(dev);
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      if (scanning) {
        manager.current.stopDeviceScan();
        setScanning(false);
      }
    }, 10000);
  }, [scanning]);

  // Connect to device and set up notifications
  const connect = useCallback(async () => {
    if (!device) {
      setError('No device selected. Scan first.');
      return;
    }
    try {
      const connectedDevice = await device.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();

      // Get characteristics
      const services = await connectedDevice.services();
      const targetService = services.find(s => s.uuid === SERVICE_UUID);
      if (!targetService) {
        setError('ACOUSTIC-PHANTOM service not found');
        return;
      }

      const characteristics = await targetService.characteristics();

      // Subscribe to event notifications
      const eventChar = characteristics.find(c => c.uuid === EVENT_CHARACTERISTIC_UUID);
      if (eventChar) {
        eventChar.monitor((err, char) => {
          if (err || !char?.value) return;
          const decoded = decodeEvent(char.value);
          if (decoded) {
            setEvents(prev => [...prev.slice(-199), decoded]);
          }
        });
      }

      // Subscribe to status notifications
      const statusChar = characteristics.find(c => c.uuid === STATUS_CHARACTERISTIC_UUID);
      if (statusChar) {
        statusChar.monitor((err, char) => {
          if (err || !char?.value) return;
          const decoded = decodeStatus(char.value);
          if (decoded) setStatus(decoded);
        });
      }

      // Subscribe to audio notifications
      const audioChar = characteristics.find(c => c.uuid === AUDIO_CHARACTERISTIC_UUID);
      if (audioChar) {
        audioChar.monitor((err, char) => {
          if (err || !char?.value) return;
          const decoded = decodeAudio(char.value);
          if (decoded) setAudioData(decoded);
        });
      }

      // Subscribe to file list notifications
      const fileChar = characteristics.find(c => c.uuid === FILE_CHARACTERISTIC_UUID);
      if (fileChar) {
        fileChar.monitor((err, char) => {
          if (err || !char?.value) return;
          const decoded = decodeFileList(char.value);
          if (decoded) setFileList(decoded);
        });
      }

      // Subscribe to calibration progress
      const calibChar = characteristics.find(c => c.uuid === CALIB_CHARACTERISTIC_UUID);
      if (calibChar) {
        calibChar.monitor((err, char) => {
          if (err || !char?.value) return;
          const val = Buffer.from(char.value, 'base64');
          setCalibProgress({
            current: val[0],
            total: val[1],
          });
        });
      }

      setConnected(true);
    } catch (err) {
      setError(err.message);
    }
  }, [device]);

  // Send a command to the device
  const sendCommand = useCallback(async (cmd, data) => {
    if (!device || !connected) return;
    try {
      const services = await device.services();
      const targetService = services.find(s => s.uuid === SERVICE_UUID);
      const characteristics = await targetService.characteristics();
      const cmdChar = characteristics.find(c => c.uuid === CMD_CHARACTERISTIC_UUID);

      if (cmdChar) {
        const encoded = encodeCommand(cmd, data);
        await cmdChar.writeWithResponse(encoded);
      }
    } catch (err) {
      setError(err.message);
    }
  }, [device, connected]);

  // Disconnect
  const disconnect = useCallback(async () => {
    if (device && connected) {
      await device.cancelConnection();
      setConnected(false);
      setStatus(null);
      setEvents([]);
    }
  }, [device, connected]);

  // Clear events
  const clearEvents = useCallback(() => setEvents([]), []);

  const value = {
    scanning,
    connected,
    status,
    events,
    audioData,
    fileList,
    calibProgress,
    error,
    scan,
    connect,
    disconnect,
    sendCommand,
    clearEvents,
  };

  return <BLEContext.Provider value={value}>{children}</BLEContext.Provider>;
}