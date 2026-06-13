/**
 * WFP-100 Companion App — Main Entry Point
 * React Native app for controlling the WFP-100 WiFi Pentesting Dongle.
 *
 * Features:
 * - Connect to WFP-100 via USB (CDC-ACM serial) or Bluetooth LE
 * - Start/stop monitor mode capture
 * - Live packet capture display with filtering
 * - Channel selection (2.4/5/6 GHz bands)
 * - Frame injection controls
 * - PCAP export to phone storage
 *
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import DeviceScreen from './screens/DeviceScreen';
import CaptureScreen from './screens/CaptureScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createNativeStackNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator
        initialRouteName="Device"
        screenOptions={{
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#00ff88',
          headerTitleStyle: { fontWeight: 'bold' },
          contentStyle: { backgroundColor: '#16213e' },
        }}
      >
        <Stack.Screen
          name="Device"
          component={DeviceScreen}
          options={{ title: 'WFP-100' }}
        />
        <Stack.Screen
          name="Capture"
          component={CaptureScreen}
          options={{ title: 'Packet Capture' }}
        />
        <Stack.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ title: 'Settings' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}