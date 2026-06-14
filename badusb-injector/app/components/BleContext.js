/**
 * PHANTOM — BLE Context Provider
 * Manages Bluetooth Low Energy connection state and communication
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under MIT
 */

import React, { createContext, useState, useEffect, useRef, useCallback } from 'react';
import { Platform, PermissionsAndroid } from 'react-native';
import { BleManager } from 'react-native-ble-plx';

// BLE Service and Characteristic UUIDs
const PHANTOM_SERVICE_UUID = '4a470000-5a5b-6c6d-7e7f-8a8b9c9d0e0f';
const PHANTOM_CHAR_CONTROL = '4a470001-5a5b-6c6d-7e7f-8a8b9c9d0e0f';
const PHANTOM_CHAR_DATA = '4a470002-5a5b-6c6d-7e7f-8a8b9c9d0e0f';
const PHANTOM_CHAR_CONFIG = '4a470003-5a5b-6c6d-7e7f-8a8b9c9d0e0f';
const PHANTOM_CHAR_STATUS = '4a470004-5a5b-6c6d-7e7f-8a8b9c9d0e0f';

// Command protocol
const CMD_PREFIX = 'PNT:';
const CMD_EXECUTE = 'EXEC';
const CMD_PROFILE_LIST = 'PLIST';
const CMD_PROFILE_GET = 'PGET';
const CMD_PROFILE_UPLOAD = 'PUP';
const CMD_PROFILE_DELETE = 'PDEL';
const CMD_GET_SETTINGS = 'GSET';
const CMD_SET_SETTINGS = 'SSET';
const CMD_SWITCH_MODE = 'SMOD';
const CMD_WIFI_SCAN = 'WSCAN';
const CMD_WIFI_CONNECT = 'WCONN';
const CMD_WIFI_DISCONNECT = 'WDISC';
const CMD_FACTORY_RESET = 'FRST';

export const BleContext = createContext({});

export const BleProvider = ({ children }) => {
  const managerRef = useRef(null);
  const [devices, setDevices] = useState([]);
  const [connectedDevice, setConnectedDevice] = useState(null);
  const [isScanning, setIsScanning] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [batteryLevel, setBatteryLevel] = useState(0);
  const [deviceState, setDeviceState] = useState('UNKNOWN');
  const [connectionError, setConnectionError] = useState(null);

  // BLE MTU and chunking
  const MTU_SIZE = 20; // Safe BLE MTU for data
  const CHUNK_DELAY = 10; // ms between chunks

  useEffect(() => {
    managerRef.current = new BleManager();
    return () => {
      if (managerRef.current) {
        managerRef.current.destroy();
      }
    };
  }, []);

  // Request BLE permissions (Android)
  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        (g) => g === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  };

  // Scan for PHANTOM devices
  const scan = useCallback(async () => {
    if (!managerRef.current) return;
    if (isScanning) return;

    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      setConnectionError('Bluetooth permissions not granted');
      return;
    }

    setIsScanning(true);
    setDevices([]);
    setConnectionError(null);

    const foundDevices = new Map();

    managerRef.current.startDeviceScan(
      [PHANTOM_SERVICE_UUID],
      null,
      (error, device) => {
        if (error) {
          setConnectionError(error.message);
          setIsScanning(false);
          return;
        }

        if (device && !foundDevices.has(device.id)) {
          foundDevices.set(device.id, {
            id: device.id,
            name: device.name || 'PHANTOM',
            rssi: device.rssi,
            device, // Store raw device for connection
          });
          setDevices(Array.from(foundDevices.values()));
        }
      }
    );

    // Stop scan after 10 seconds
    setTimeout(() => {
      managerRef.current?.stopDeviceScan();
      setIsScanning(false);
    }, 10000);
  }, [isScanning]);

  // Connect to a device
  const connect = useCallback(async (deviceEntry) => {
    if (!managerRef.current || !deviceEntry?.device) return;

    try {
      const connected = await deviceEntry.device.connect({
        timeout: 10000,
        requestMTU: 247,
      });

      // Discover services and characteristics
      await connected.discoverAllServicesAndCharacteristics();

      setConnectedDevice(connected);

      // Start monitoring status characteristic
      connected.monitorCharacteristicForService(
        PHANTOM_SERVICE_UUID,
        PHANTOM_CHAR_STATUS,
        (error, characteristic) => {
          if (error) {
            console.error('Status monitoring error:', error);
            return;
          }

          if (characteristic?.value) {
            const decoded = atob(characteristic.value);
            handleStatusUpdate(decoded);
          }
        }
      );

      // Read initial device info
      const infoChar = await connected.readCharacteristicForService(
        PHANTOM_SERVICE_UUID,
        PHANTOM_CHAR_STATUS
      );
      if (infoChar?.value) {
        const decoded = atob(infoChar.value);
        handleStatusUpdate(decoded);
      }
    } catch (error) {
      setConnectionError(error.message);
      throw error;
    }
  }, []);

  // Handle status updates from device
  const handleStatusUpdate = (data) => {
    try {
      const status = JSON.parse(data);
      setDeviceInfo({
        name: status.name || 'PHANTOM',
        firmwareVersion: status.fw_version || '1.0.0',
        profileCount: status.profile_count || 0,
        serial: status.serial || '',
      });
      setBatteryLevel(status.battery || 0);
      setDeviceState(status.state || 'UNKNOWN');
    } catch (e) {
      // Not JSON, try simple state format
      if (['STEALTH', 'HID', 'EXECUTING', 'GEOFENCE', 'KILL', 'ERROR'].includes(data)) {
        setDeviceState(data);
      }
    }
  };

  // Disconnect from device
  const disconnect = useCallback(async () => {
    if (connectedDevice) {
      try {
        await connectedDevice.cancelConnection();
      } catch (e) {
        // Ignore disconnect errors
      }
      setConnectedDevice(null);
      setDeviceInfo(null);
      setBatteryLevel(0);
      setDeviceState('UNKNOWN');
    }
  }, [connectedDevice]);

  // Send a command to the device via BLE
  const sendCommand = useCallback(
    async (command, data = null) => {
      if (!connectedDevice) {
        throw new Error('Not connected to device');
      }

      const payload = data ? JSON.stringify(data) : '';
      const message = `${CMD_PREFIX}${command}:${payload}`;

      // Chunk the message if it exceeds MTU
      const chunks = [];
      for (let i = 0; i < message.length; i += MTU_SIZE) {
        chunks.push(message.slice(i, i + MTU_SIZE));
      }

      // Select characteristic based on command type
      let characteristic = PHANTOM_CHAR_CONTROL;
      if (command.startsWith('P')) {
        characteristic = PHANTOM_CHAR_DATA;
      } else if (command.startsWith('S')) {
        characteristic = PHANTOM_CHAR_CONFIG;
      }

      // Send each chunk
      for (let i = 0; i < chunks.length; i++) {
        const encoded = btoa(chunks[i]);
        await connectedDevice.writeCharacteristicWithResponseForService(
          PHANTOM_SERVICE_UUID,
          characteristic,
          encoded
        );

        // Small delay between chunks
        if (i < chunks.length - 1) {
          await new Promise((resolve) => setTimeout(resolve, CHUNK_DELAY));
        }
      }

      // Read response
      const response = await connectedDevice.readCharacteristicForService(
        PHANTOM_SERVICE_UUID,
        characteristic
      );

      if (response?.value) {
        const decoded = atob(response.value);
        try {
          return JSON.parse(decoded);
        } catch {
          return decoded;
        }
      }

      return null;
    },
    [connectedDevice]
  );

  // Auto-reconnect
  useEffect(() => {
    if (!connectedDevice) return;

    const subscription = connectedDevice.onDisconnected(() => {
      setConnectedDevice(null);
      setDeviceInfo(null);
      setDeviceState('UNKNOWN');
    });

    return () => subscription.remove();
  }, [connectedDevice]);

  const value = {
    devices,
    connectedDevice,
    isScanning,
    deviceInfo,
    batteryLevel,
    deviceState,
    connectionError,
    scan,
    connect,
    disconnect,
    sendCommand,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
};

export default BleContext;