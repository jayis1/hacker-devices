/**
 * App.js — PlasmaReaper Companion App
 * Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: MIT
 *
 * Main application component with bottom-tab navigation:
 *  - Connect: device connection (USB/BLE)
 *  - Manual: manual glitch parameter editor + single shot
 *  - Sweep: 2-D parameter sweep configurator + heat-map
 *  - Results: sweep results dashboard
 *  - Settings: patterns, trigger config, logs
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import ConnectScreen from './screens/ConnectScreen';
import ManualScreen from './screens/ManualScreen';
import SweepScreen from './screens/SweepScreen';
import ResultsScreen from './screens/ResultsScreen';
import SettingsScreen from './screens/SettingsScreen';

import { DeviceContext } from './utils/deviceContext';

const Tab = createBottomTabNavigator();

const TAB_ICONS = {
  Connect: 'usb',
  Manual: 'flash',
  Sweep: 'grid',
  Results: 'chart-bar',
  Settings: 'cog',
};

function screenOptions({ route }) {
  return {
    tabBarIcon: ({ color, size }) => (
      <Icon name={TAB_ICONS[route.name] || 'help'} color={color} size={size} />
    ),
    tabBarActiveTintColor: '#e91e63',
    tabBarInactiveTintColor: 'gray',
    headerTitleStyle: { fontWeight: 'bold' },
    headerTitle: 'PlasmaReaper',
  };
}

export default function App() {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState({
    totalShots: 0,
    successShots: 0,
    failureShots: 0,
    sweepRunning: false,
  });

  const connectDevice = useCallback(async (type, params) => {
    try {
      if (type === 'usb') {
        // USB CDC connection (Android only, via react-native-usb-serial)
        const UsbSerial = require('react-native-usb-serial').default;
        const dev = await UsbSerial.connect(params.deviceId, 115200);
        setDevice({ type: 'usb', transport: dev });
        setConnected(true);
        return true;
      } else if (type === 'ble') {
        // BLE connection (iOS/Android, via react-native-ble-plx)
        const { BleManager } = require('react-native-ble-plx');
        const bleManager = new BleManager();
        const dev = await bleManager.connectToDevice(params.deviceId);
        await dev.discoverAllServicesAndCharacteristics();
        setDevice({ type: 'ble', transport: dev, bleManager });
        setConnected(true);
        return true;
      }
      return false;
    } catch (err) {
      console.error('Connection failed:', err);
      setConnected(false);
      return false;
    }
  }, []);

  const disconnectDevice = useCallback(async () => {
    if (device) {
      try {
        if (device.type === 'usb') {
          await device.transport.disconnect();
        } else if (device.type === 'ble') {
          await device.transport.cancelConnection();
        }
      } catch (err) {
        console.error('Disconnect error:', err);
      }
    }
    setDevice(null);
    setConnected(false);
  }, [device]);

  const sendCommand = useCallback(async (cmd, payload) => {
    if (!device || !connected) return null;

    const frame = buildFrame(cmd, payload);
    let response;

    if (device.type === 'usb') {
      response = await device.transport.write(frame);
    } else if (device.type === 'ble') {
      // Write to the BLE characteristic
      const SERVICE_UUID = '0000ffe0-0000-1000-8000-00805f9b34fb';
      const CHAR_UUID = '0000ffe1-0000-1000-8000-00805f9b34fb';
      await device.transport.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CHAR_UUID, frame, 'base64'
      );
      response = await readBleResponse(device.transport, SERVICE_UUID, CHAR_UUID);
    }

    return parseFrame(response);
  }, [device, connected]);

  // Poll status every 2 seconds when connected
  useEffect(() => {
    if (!connected) return;

    const interval = setInterval(async () => {
      const result = await sendCommand(CMD_GET_STATUS, null);
      if (result && result.type === RESP_STATUS) {
        setStatus({
          totalShots: result.data.totalShots,
          successShots: result.data.successShots,
          failureShots: result.data.failureShots,
          sweepRunning: result.data.sweepRunning,
          sweepProgress: result.data.completedCells / result.data.totalCells,
        });
      }
    }, 2000);

    return () => clearInterval(interval);
  }, [connected, sendCommand]);

  const contextValue = {
    device,
    connected,
    status,
    connectDevice,
    disconnectDevice,
    sendCommand,
  };

  return (
    <DeviceContext.Provider value={contextValue}>
      <NavigationContainer>
        <Tab.Navigator screenOptions={screenOptions}>
          <Tab.Screen name="Connect" component={ConnectScreen} />
          <Tab.Screen name="Manual" component={ManualScreen} />
          <Tab.Screen name="Sweep" component={SweepScreen} />
          <Tab.Screen name="Results" component={ResultsScreen} />
          <Tab.Screen name="Settings" component={SettingsScreen} />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceContext.Provider>
  );
}

// ---- Protocol helpers ----
import {
  CMD_GET_STATUS, RESP_STATUS,
  buildFrame, parseFrame, readBleResponse,
} from './utils/protocol';