/**
 * App.js — PhantomBridge PoE Network Implant Companion App
 * React Native navigation root with BLE connection management
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { BleManager } from 'react-native-ble-plx';
import { Provider as PaperProvider, DarkTheme } from 'react-native-paper';

import DashboardScreen from './screens/DashboardScreen';
import RulesScreen from './screens/RulesScreen';
import CaptureScreen from './screens/CaptureScreen';
import { BLEContext, BLE_SERVICE_UUID, connectToDevice } from './utils/protocol';

const Stack = createNativeStackNavigator();

const darkTheme = {
  ...DarkTheme,
  colors: {
    ...DarkTheme.colors,
    primary: '#00e676',
    accent: '#ff5252',
    background: '#121212',
    surface: '#1e1e1e',
  },
};

export default function App() {
  const [bleManager] = useState(new BleManager());
  const [connectedDevice, setConnectedDevice] = useState(null);
  const [connectionState, setConnectionState] = useState('disconnected'); // disconnected, scanning, connecting, connected
  const [telemetry, setTelemetry] = useState({
    poeVoltage: 0,
    poeCurrent: 0,
    poePower: 0,
    rxPackets: 0,
    txPackets: 0,
    modifiedPackets: 0,
    droppedPackets: 0,
    captureUsage: 0,
    uptime: 0,
  });

  // Scan and connect to PhantomBridge device
  const scanAndConnect = async () => {
    setConnectionState('scanning');
    try {
      bleManager.startDeviceScan(
        [BLE_SERVICE_UUID],
        null,
        async (error, device) => {
          if (error) {
            console.error('BLE scan error:', error);
            setConnectionState('disconnected');
            return;
          }
          if (device.name && device.name.includes('PhantomBridge')) {
            bleManager.stopDeviceScan();
            setConnectionState('connecting');
            const connected = await connectToDevice(device);
            setConnectedDevice(connected);
            setConnectionState('connected');
          }
        }
      );
    } catch (e) {
      console.error('Connection failed:', e);
      setConnectionState('disconnected');
    }
  };

  // Disconnect
  const disconnect = async () => {
    if (connectedDevice) {
      await connectedDevice.cancelConnection();
      setConnectedDevice(null);
      setConnectionState('disconnected');
    }
  };

  // Periodic telemetry poll
  useEffect(() => {
    if (connectionState !== 'connected' || !connectedDevice) return;

    const interval = setInterval(async () => {
      try {
        const data = await readTelemetry(connectedDevice);
        if (data) setTelemetry(data);
      } catch (e) {
        console.error('Telemetry read failed:', e);
      }
    }, 2000);

    return () => clearInterval(interval);
  }, [connectionState, connectedDevice]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      bleManager.destroy();
    };
  }, []);

  const bleContextValue = {
    manager: bleManager,
    device: connectedDevice,
    connectionState,
    telemetry,
    scanAndConnect,
    disconnect,
  };

  return (
    <PaperProvider theme={darkTheme}>
      <BLEContext.Provider value={bleContextValue}>
        <NavigationContainer theme={darkTheme}>
          <Stack.Navigator
            initialRouteName="Dashboard"
            screenOptions={{
              headerStyle: { backgroundColor: '#1e1e1e' },
              headerTintColor: '#00e676',
              headerTitleStyle: { fontWeight: 'bold' },
            }}
          >
            <Stack.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{ title: 'PhantomBridge' }}
            />
            <Stack.Screen
              name="Rules"
              component={RulesScreen}
              options={{ title: 'Packet Rules' }}
            />
            <Stack.Screen
              name="Capture"
              component={CaptureScreen}
              options={{ title: 'Capture Buffer' }}
            />
          </Stack.Navigator>
        </NavigationContainer>
      </BLEContext.Provider>
    </PaperProvider>
  );
}

// Helper: read telemetry from BLE characteristic
async function readTelemetry(device) {
  try {
    const characteristic = await device.readCharacteristicForService(
      BLE_SERVICE_UUID,
      '00002A6E-0000-1000-8000-00805F9B34FB' // Telemetry characteristic
    );
    if (!characteristic.value) return null;

    const bytes = base64ToBytes(characteristic.value);
    return {
      poeVoltage: bytesToFloat(bytes, 0),
      poeCurrent: bytesToFloat(bytes, 4),
      poePower: bytesToFloat(bytes, 8),
      rxPackets: bytesToUint32(bytes, 12),
      txPackets: bytesToUint32(bytes, 16),
      modifiedPackets: bytesToUint32(bytes, 20),
      droppedPackets: bytesToUint32(bytes, 24),
      captureUsage: bytesToUint32(bytes, 28),
      uptime: bytesToUint32(bytes, 32),
    };
  } catch {
    return null;
  }
}

function base64ToBytes(base64) {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}

function bytesToFloat(bytes, offset) {
  const view = new DataView(bytes.buffer, offset, 4);
  return view.getFloat32(0, true); // little-endian
}

function bytesToUint32(bytes, offset) {
  return (
    bytes[offset] |
    (bytes[offset + 1] << 8) |
    (bytes[offset + 2] << 16) |
    (bytes[offset + 3] << 24)
  ) >>> 0;
}