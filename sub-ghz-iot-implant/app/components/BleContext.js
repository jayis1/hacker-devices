/**
 * BleContext.js — BLE Connection Manager
 * Provides React context for BLE state management and communication
 */

import React, { createContext, useState, useRef, useCallback } from 'react';
import { Platform, PermissionsAndroid } from 'react-native';
import BleManager from 'react-native-ble-plx';

// BLE Service and Characteristic UUIDs (matching firmware)
const SERVICE_UUID = '53550001-5355-5355-5355-535553555355';
const CHAR_MODE_UUID = '53550002-5355-5355-5355-535553555355';
const CHAR_CAPTURE_UUID = '53550003-5355-5355-5355-535553555355';
const CHAR_CONFIG_UUID = '53550004-5355-5355-5355-535553555355';
const CHAR_STATUS_UUID = '53550005-5355-5355-5355-535553555355';

export const BleContext = createContext({
  device: null,
  connectionState: 'disconnected',
  connect: async () => {},
  disconnect: async () => {},
  scanForDevices: async () => [],
  startCapture: () => {},
  stopCapture: () => {},
  isCapturing: false,
  packets: [],
  clearPackets: () => {},
  sendCommand: () => {},
  deviceInfo: null,
  config: {},
  updateConfig: () => {},
});

export function BleProvider({ children }) {
  const bleManager = useRef(new BleManager()).current;
  const [device, setDevice] = useState(null);
  const [connectionState, setConnectionState] = useState('disconnected');
  const [isCapturing, setIsCapturing] = useState(false);
  const [packets, setPackets] = useState([]);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [config, setConfig] = useState({
    frequency: 868000000,
    modulation: 2,
    txPower: 14,
    mode: 'idle',
    channelBW: 200000,
    deviation: 25000,
    syncWord: 'AABBCCDD',
  });
  const [scanResults, setScanResults] = useState([]);
  const subscriptionRef = useRef(null);

  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      const apiLevel = Platform.Version;
      if (apiLevel >= 31) {
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        ]);
        return Object.values(granted).every(
          (v) => v === PermissionsAndroid.RESULTS.GRANTED
        );
      } else {
        const granted = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION
        );
        return granted === PermissionsAndroid.RESULTS.GRANTED;
      }
    }
    return true; // iOS permissions handled by the library
  };

  const scanForDevices = useCallback(async () => {
    const hasPermission = await requestPermissions();
    if (!hasPermission) return [];

    setScanResults([]);
    const found = [];

    return new Promise((resolve) => {
      bleManager.startDeviceScan(
        [SERVICE_UUID],
        { allowDuplicates: false },
        (error, scannedDevice) => {
          if (error) {
            console.error('BLE scan error:', error);
            resolve([]);
            return;
          }
          if (scannedDevice && !found.find((d) => d.id === scannedDevice.id)) {
            found.push({
              id: scannedDevice.id,
              name: scannedDevice.name || 'SUBSTATION',
              rssi: scannedDevice.rssi,
            });
            setScanResults([...found]);
          }
        }
      );

      // Stop scan after 5 seconds
      setTimeout(() => {
        bleManager.stopDeviceScan();
        resolve(found);
      }, 5000);
    });
  }, []);

  const connect = useCallback(async (deviceId) => {
    try {
      setConnectionState('connecting');
      const connectedDevice = await bleManager.connectToDevice(deviceId);
      await bleManager.discoverAllServicesAndCharacteristicsForDevice(deviceId);
      setDevice(connectedDevice);
      setConnectionState('connected');

      // Subscribe to status notifications
      subscriptionRef.current = bleManager.monitorCharacteristicForDevice(
        deviceId,
        SERVICE_UUID,
        CHAR_STATUS_UUID,
        (error, characteristic) => {
          if (error) {
            console.error('Status notification error:', error);
            return;
          }
          if (characteristic?.value) {
            const data = base64Decode(characteristic.value);
            setDeviceInfo({
              mode: data[0],
              frequency: (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4],
              rssi: data[5] > 127 ? data[5] - 256 : data[5],
              packetCount: (data[6] << 24) | (data[7] << 16) | (data[8] << 8) | data[9],
              uptime: (data[10] << 24) | (data[11] << 16) | (data[12] << 8) | data[13],
              battery: data[14],
            });
          }
        }
      );

      // Also subscribe to capture notifications
      bleManager.monitorCharacteristicForDevice(
        deviceId,
        SERVICE_UUID,
        CHAR_CAPTURE_UUID,
        (error, characteristic) => {
          if (error) {
            console.error('Capture notification error:', error);
            return;
          }
          if (characteristic?.value) {
            const data = base64Decode(characteristic.value);
            const packet = parsePacket(data);
            setPackets((prev) => [...prev.slice(-499), packet]); // Keep last 500
          }
        }
      );
    } catch (error) {
      console.error('BLE connect error:', error);
      setConnectionState('disconnected');
    }
  }, []);

  const disconnect = useCallback(async () => {
    try {
      if (subscriptionRef.current) {
        subscriptionRef.current.remove();
        subscriptionRef.current = null;
      }
      if (device) {
        await bleManager.cancelDeviceConnection(device.id);
      }
      setDevice(null);
      setConnectionState('disconnected');
      setDeviceInfo(null);
      setIsCapturing(false);
    } catch (error) {
      console.error('BLE disconnect error:', error);
    }
  }, [device]);

  const startCapture = useCallback(async () => {
    if (!device || connectionState !== 'connected') return;
    try {
      await bleManager.writeCharacteristicWithResponseForDevice(
        device.id,
        SERVICE_UUID,
        CHAR_MODE_UUID,
        base64Encode([0x01]) // Start capture
      );
      setIsCapturing(true);
    } catch (error) {
      console.error('Start capture error:', error);
    }
  }, [device, connectionState]);

  const stopCapture = useCallback(async () => {
    if (!device || connectionState !== 'connected') return;
    try {
      await bleManager.writeCharacteristicWithResponseForDevice(
        device.id,
        SERVICE_UUID,
        CHAR_MODE_UUID,
        base64Encode([0x00]) // Stop capture
      );
      setIsCapturing(false);
    } catch (error) {
      console.error('Stop capture error:', error);
    }
  }, [device, connectionState]);

  const clearPackets = useCallback(() => {
    setPackets([]);
  }, []);

  const sendCommand = useCallback(
    async (command, params = {}) => {
      if (!device || connectionState !== 'connected') return;
      try {
        let data;
        switch (command) {
          case 'CONFIG':
            data = encodeConfig(params);
            break;
          case 'REPLAY':
            data = encodeReplay(params);
            break;
          default:
            data = base64Encode([0x00]);
        }
        await bleManager.writeCharacteristicWithResponseForDevice(
          device.id,
          SERVICE_UUID,
          CHAR_CONFIG_UUID,
          data
        );
      } catch (error) {
        console.error('Send command error:', error);
      }
    },
    [device, connectionState]
  );

  const updateConfig = useCallback((newConfig) => {
    setConfig((prev) => ({ ...prev, ...newConfig }));
  }, []);

  const value = {
    device,
    connectionState,
    connect,
    disconnect,
    scanForDevices,
    startCapture,
    stopCapture,
    isCapturing,
    packets,
    clearPackets,
    sendCommand,
    deviceInfo,
    config,
    updateConfig,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

// Helper functions
function base64Decode(base64) {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}

function base64Encode(bytes) {
  let binary = '';
  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i]);
  }
  return btoa(binary);
}

function parsePacket(data) {
  return {
    id: Date.now(),
    timestamp: (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3],
    rssi: data[4] > 127 ? data[4] - 256 : data[4],
    channel: data[5],
    length: data[6],
    protocol: detectProtocol(data),
    crcValid: data[data.length - 1] === 1,
    rawData: Array.from(data)
      .slice(7, 7 + data[6])
      .map((b) => b.toString(16).padStart(2, '0'))
      .join(' '),
    modulation: data.length > 7 + data[6] + 1 ? data[7 + data[6] + 1] : 0,
    frequency: data.length > 7 + data[6] + 5
      ? (data[7 + data[6] + 2] << 24) |
        (data[7 + data[6] + 3] << 16) |
        (data[7 + data[6] + 4] << 8) |
        data[7 + data[6] + 5]
      : 0,
  };
}

function detectProtocol(data) {
  // Simple heuristic based on packet structure
  const len = data[6];
  if (len === 0) return 'unknown';
  // Check for IEEE 802.15.4 frame control
  if (len >= 2 && (data[7] & 0x07) === 0x01) return 'zigbee';
  // Check for Z-Wave home ID pattern
  if (len >= 4 && data[7] >= 0x01 && data[7] <= 0x09) return 'zwave';
  return 'subghz';
}

function encodeConfig(params) {
  const buf = new Uint8Array(20);
  buf[0] = 0x01; // Command: CONFIG
  buf[1] = (params.frequency >> 24) & 0xFF;
  buf[2] = (params.frequency >> 16) & 0xFF;
  buf[3] = (params.frequency >> 8) & 0xFF;
  buf[4] = params.frequency & 0xFF;
  buf[5] = params.modulation || 0;
  buf[6] = params.tx_power || 14;
  buf[7] = (params.channel_bw >> 24) & 0xFF;
  buf[8] = (params.channel_bw >> 16) & 0xFF;
  buf[9] = (params.channel_bw >> 8) & 0xFF;
  buf[10] = params.channel_bw & 0xFF;
  buf[11] = (params.deviation >> 24) & 0xFF;
  buf[12] = (params.deviation >> 16) & 0xFF;
  buf[13] = (params.deviation >> 8) & 0xFF;
  buf[14] = params.deviation & 0xFF;
  // Sync word (4 bytes, hex string)
  const sw = parseInt(params.sync_word || 'AABBCCDD', 16);
  buf[15] = (sw >> 24) & 0xFF;
  buf[16] = (sw >> 16) & 0xFF;
  buf[17] = (sw >> 8) & 0xFF;
  buf[18] = sw & 0xFF;
  buf[19] = 0x00; // Reserved
  return base64Encode(buf);
}

function encodeReplay(params) {
  const dataLen = params.data ? params.data.split(' ').length : 0;
  const buf = new Uint8Array(10 + dataLen);
  buf[0] = 0x02; // Command: REPLAY
  buf[1] = (params.frequency >> 24) & 0xFF;
  buf[2] = (params.frequency >> 16) & 0xFF;
  buf[3] = (params.frequency >> 8) & 0xFF;
  buf[4] = params.frequency & 0xFF;
  buf[5] = params.modulation || 0;
  buf[6] = (dataLen >> 8) & 0xFF;
  buf[7] = dataLen & 0xFF;
  if (params.data) {
    const bytes = params.data.split(' ');
    for (let i = 0; i < bytes.length && i + 8 < buf.length; i++) {
      buf[8 + i] = parseInt(bytes[i], 16);
    }
  }
  return base64Encode(buf);
}