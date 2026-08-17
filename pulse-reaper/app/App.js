/**
 * App.js — Pulse-Reaper companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Main navigation and app structure for the Pulse-Reaper TDR-guided
 * non-invasive covert tap companion app. Provides BLE connection
 * management and navigation to all TDR / sniff / inject / cable-map
 * screens.
 *
 * Legal: for authorized security research only. See README disclaimer.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import { DeviceProvider } from './components/DeviceContext';
import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import TDRReflectogramScreen from './screens/TDRReflectogramScreen';
import LiveCaptureScreen from './screens/LiveCaptureScreen';
import CableMapScreen from './screens/CableMapScreen';
import InjectScreen from './screens/InjectScreen';
import CovertChannelScreen from './screens/CovertChannelScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createStackNavigator();

/**
 * Root component for the Pulse-Reaper app.
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
            headerTintColor: '#ff6b35',
            headerTitleStyle: { fontWeight: 'bold' },
            cardStyle: { backgroundColor: '#0d1117' },
          }}
        >
          <Stack.Screen
            name="Connection"
            component={ConnectionScreen}
            options={{ title: 'Pulse-Reaper — by jayis1' }}
          />
          <Stack.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'Pulse-Reaper Dashboard' }}
          />
          <Stack.Screen
            name="TDRReflectogram"
            component={TDRReflectogramScreen}
            options={{ title: 'TDR Reflectogram' }}
          />
          <Stack.Screen
            name="LiveCapture"
            component={LiveCaptureScreen}
            options={{ title: 'Live Capture' }}
          />
          <Stack.Screen
            name="CableMap"
            component={CableMapScreen}
            options={{ title: 'Cable Map' }}
          />
          <Stack.Screen
            name="Inject"
            component={InjectScreen}
            options={{ title: 'Inject' }}
          />
          <Stack.Screen
            name="CovertChannel"
            component={CovertChannelScreen}
            options={{ title: 'Covert Channel' }}
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