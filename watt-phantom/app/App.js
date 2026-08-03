/**
 * App.js — WattPhantom companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Main navigation and app structure for the WattPhantom USB-C Power Delivery
 * manipulation and covert channel platform companion app. Provides BLE
 * connection management and navigation to all attack/monitoring screens.
 *
 * Legal: for authorized security research only. See README disclaimer.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import { DeviceProvider } from './components/DeviceContext';
import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import PDManipulatorScreen from './screens/PDManipulatorScreen';
import CovertChannelScreen from './screens/CovertChannelScreen';
import FingerprintScreen from './screens/FingerprintScreen';
import PowerMonitorScreen from './screens/PowerMonitorScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createStackNavigator();

/**
 * Root component for the WattPhantom app.
 * Author: jayis1
 */
export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Connection"
          screenOptions={{
            headerStyle: { backgroundColor: '#0d1117' },
            headerTintColor: '#00d4aa',
            headerTitleStyle: { fontWeight: 'bold' },
            cardStyle: { backgroundColor: '#0d1117' },
          }}
        >
          <Stack.Screen
            name="Connection"
            component={ConnectionScreen}
            options={{ title: 'WattPhantom — by jayis1' }}
          />
          <Stack.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'WattPhantom Dashboard' }}
          />
          <Stack.Screen
            name="PDManipulator"
            component={PDManipulatorScreen}
            options={{ title: 'PD Manipulator' }}
          />
          <Stack.Screen
            name="CovertChannel"
            component={CovertChannelScreen}
            options={{ title: 'CC Covert Channel' }}
          />
          <Stack.Screen
            name="Fingerprint"
            component={FingerprintScreen}
            options={{ title: 'Device Fingerprint' }}
          />
          <Stack.Screen
            name="PowerMonitor"
            component={PowerMonitorScreen}
            options={{ title: 'Power Monitor' }}
          />
          <Stack.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}