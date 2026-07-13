/**
 * eddy-phantom — App.js
 * Main React Native application entry point for the
 * Eddy-Phantom electromagnetic emanation keyboard reconnaissance device.
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
import LiveFeedScreen from './screens/LiveFeedScreen';
import SessionLogScreen from './screens/SessionLogScreen';
import ProfileScreen from './screens/ProfileScreen';
import RawCaptureScreen from './screens/RawCaptureScreen';
import SettingsScreen from './screens/SettingsScreen';

// BLE service and characteristic UUIDs
export const EDDY_SERVICE_UUID    = 'f0000001-0000-1000-8000-00805f9b34fb';
export const EDDY_CHAR_KEYSTREAM  = 'f0000002-0000-1000-8000-00805f9b34fb';
export const EDDY_CHAR_COMMAND    = 'f0000003-0000-1000-8000-00805f9b34fb';
export const EDDY_CHAR_STATUS     = 'f0000004-0000-1000-8000-00805f9b34fb';
export const EDDY_CHAR_RAWBURST   = 'f0000005-0000-1000-8000-00805f9b34fb';
export const EDDY_CHAR_AUTH       = 'f0000006-0000-1000-8000-00805f9b34fb';

// Global BLE manager instance
const bleManager = new BLEManager();

// Navigation stack
const Stack = createStackNavigator();

export default function App() {
  const [connected, setConnected] = useState(false);
  const [deviceState, setDeviceState] = useState('IDLE');
  const [battery, setBattery] = useState(0);
  const [profile, setProfile] = useState('');
  const [keystrokes, setKeystrokes] = useState([]);
  const [sessionLog, setSessionLog] = useState([]);

  // BLE connection state callback
  useEffect(() => {
    bleManager.onConnectionChange = (isConn) => {
      setConnected(isConn);
    };
    bleManager.onStatusUpdate = (state, batt, prof) => {
      setDeviceState(state);
      setBattery(batt);
      setProfile(prof);
    };
    bleManager.onKeystroke = (scancode, confidence, timestamp) => {
      const char = scancodeToAscii(scancode);
      const entry = {
        id: Date.now() + Math.random(),
        char,
        scancode,
        confidence,
        timestamp,
        time: new Date(timestamp).toLocaleTimeString(),
      };
      setKeystrokes(prev => [...prev.slice(-500), entry]);
      setSessionLog(prev => [...prev.slice(-1000), entry]);
    };
  }, []);

  // Expose BLE manager to screens via navigation params
  const screenProps = {
    bleManager,
    connected,
    deviceState,
    battery,
    profile,
    keystrokes,
    sessionLog,
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
          <Stack.Screen name="Dashboard">
            {(props) => <DashboardScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="Live Feed" options={{ title: 'Live Feed' }}>
            {(props) => <LiveFeedScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="Session Log">
            {(props) => <SessionLogScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="Profiles">
            {(props) => <ProfileScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="Raw Capture">
            {(props) => <RawCaptureScreen {...props} {...screenProps} />}
          </Stack.Screen>
          <Stack.Screen name="Settings">
            {(props) => <SettingsScreen {...props} {...screenProps} />}
          </Stack.Screen>
        </Stack.Navigator>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}

// USB HID scancode to ASCII mapping
function scancodeToAscii(scancode) {
  const map = {
    0x04: 'a', 0x05: 'b', 0x06: 'c', 0x07: 'd', 0x08: 'e',
    0x09: 'f', 0x0A: 'g', 0x0B: 'h', 0x0C: 'i', 0x0D: 'j',
    0x0E: 'k', 0x0F: 'l', 0x10: 'm', 0x11: 'n', 0x12: 'o',
    0x13: 'p', 0x14: 'q', 0x15: 'r', 0x16: 's', 0x17: 't',
    0x18: 'u', 0x19: 'v', 0x1A: 'w', 0x1B: 'x', 0x1C: 'y',
    0x1D: 'z', 0x1E: '1', 0x1F: '2', 0x20: '3', 0x21: '4',
    0x22: '5', 0x23: '6', 0x24: '7', 0x25: '8', 0x26: '9',
    0x27: '0', 0x28: '\n', 0x29: 'ESC', 0x2A: 'BKSP',
    0x2B: '\t', 0x2C: ' ', 0x2D: '-', 0x2E: '=', 0x2F: '[',
    0x30: ']', 0x31: '\\', 0x33: ';', 0x34: "'", 0x35: '`',
    0x36: ',', 0x37: '.', 0x38: '/',
    0xE0: 'LSHIFT', 0xE1: 'LCTRL', 0xE2: 'LALT',
  };
  return map[scancode] || `[$scancode]`;
}