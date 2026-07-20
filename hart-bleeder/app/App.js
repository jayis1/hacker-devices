/**
 * hart-bleeder — App.js
 * Main React Native application entry point for the
 * HART-Bleeder HART fieldbus covert in-line attack implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import BLEManager from './components/BLEManager';
import DashboardScreen from './screens/DashboardScreen';
import DeviceScanScreen from './screens/DeviceScanScreen';
import LiveCaptureScreen from './screens/LiveCaptureScreen';
import LoopControlScreen from './screens/LoopControlScreen';
import AttackConsoleScreen from './screens/AttackConsoleScreen';
import SessionLogScreen from './screens/SessionLogScreen';
import SettingsScreen from './screens/SettingsScreen';

// BLE GATT service and characteristic UUIDs (must match firmware c2link.h)
export const HART_SVC_UUID    = 'a0000001-0000-1000-8000-00805f9b34fb';
export const HART_CHAR_CMD    = 'a0000002-0000-1000-8000-00805f9b34fb';
export const HART_CHAR_LOG    = 'a0000003-0000-1000-8000-00805f9b34fb';
export const HART_CHAR_STATUS = 'a0000004-0000-1000-8000-00805f9b34fb';
export const HART_CHAR_AUTH   = 'a0000005-0000-1000-8000-00805f9b34fb';
export const HART_CHAR_FRAME  = 'a0000006-0000-1000-8000-00805f9b34fb';

// C2 protocol opcodes (must match c2link.h)
export const OP = {
  STATUS:      0x01,
  SCAN:        0x02,
  READ_PV:     0x03,
  SPOOF:       0x04,
  SETPOINT:    0x05,
  DOS:         0x06,
  SAG:         0x07,
  CAPTURE:     0x08,
  FUZZ:        0x09,
  COVERT_EX:   0x0A,
  COVERT_RX:   0x0B,
  MODE:        0x0C,
  PASSIVE:     0x0D,
  AUTH:        0x10,
  LOG_NOTIFY:  0x20,
};

// Global BLE manager instance
const bleManager = new BLEManager();

const Stack = createStackNavigator();

export default function App() {
  const [connected, setConnected] = useState(false);
  const [deviceState, setDeviceState] = useState('IDLE');
  const [battery, setBattery] = useState(0);
  const [nDevices, setNDevices] = useState(0);
  const [loopCurrent, setLoopCurrent] = useState(0);
  const [devices, setDevices] = useState([]);
  const [capturedFrames, setCapturedFrames] = useState([]);
  const [sessionLog, setSessionLog] = useState([]);
  const [armed, setArmed] = useState(false);

  // BLE connection / status callbacks
  useEffect(() => {
    bleManager.onConnectionChange = (isConn) => {
      setConnected(isConn);
      if (!isConn) {
        setDeviceState('IDLE');
        setArmed(false);
      }
    };
    bleManager.onStatusUpdate = (state, batt, nDev, iMa) => {
      setDeviceState(state);
      setBattery(batt);
      setNDevices(nDev);
      setLoopCurrent(iMa);
    };
    bleManager.onFrame = (frame) => {
      setCapturedFrames((prev) => [...prev.slice(-199), frame]);
    };
    bleManager.onLog = (text) => {
      setSessionLog((prev) => [...prev.slice(-499), text]);
    };
    bleManager.onScanResult = (devs) => {
      setDevices(devs);
      setNDevices(devs.length);
    };
  }, []);

  // Navigation props shared with all screens
  const screenProps = {
    bleManager,
    connected,
    deviceState,
    battery,
    nDevices,
    loopCurrent,
    devices,
    capturedFrames,
    sessionLog,
    armed,
    setArmed,
  };

  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Dashboard"
          screenOptions={{
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#e94560',
            headerTitleStyle: { fontWeight: 'bold' },
          }}
        >
          <Stack.Screen name="Dashboard" options={{ title: 'HART-Bleeder' }}>
            {(props) => <DashboardScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="DeviceScan" options={{ title: 'Bus Enumeration' }}>
            {(props) => <DeviceScanScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="LiveCapture" options={{ title: 'Live Capture' }}>
            {(props) => <LiveCaptureScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="LoopControl" options={{ title: 'Loop Control' }}>
            {(props) => <LoopControlScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="AttackConsole" options={{ title: 'Attack Console' }}>
            {(props) => <AttackConsoleScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="SessionLog" options={{ title: 'Session Log' }}>
            {(props) => <SessionLogScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="Settings" options={{ title: 'Settings' }}>
            {(props) => <SettingsScreen {...props} {...screenProps} />}
          </Stack.Screen>
        </Stack.Navigator>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}