/**
 * App.js — SideProbe companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Main navigation and app structure for the SideProbe side-channel
 * cryptanalysis platform companion app. Provides BLE connection management
 * and navigation to all attack/capture screens.
 *
 * Legal: for authorized security research only. See README disclaimer.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import { DeviceProvider } from './components/DeviceContext';
import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import CaptureScreen from './screens/CaptureScreen';
import TraceViewerScreen from './screens/TraceViewerScreen';
import AttackScreen from './screens/AttackScreen';
import PlaintextManagerScreen from './screens/PlaintextManagerScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createStackNavigator();

/**
 * Root component for the SideProbe app.
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
            options={{ title: 'SideProbe — by jayis1' }}
          />
          <Stack.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'SideProbe Dashboard' }}
          />
          <Stack.Screen
            name="Capture"
            component={CaptureScreen}
            options={{ title: 'Live Capture' }}
          />
          <Stack.Screen
            name="TraceViewer"
            component={TraceViewerScreen}
            options={{ title: 'Trace Viewer' }}
          />
          <Stack.Screen
            name="Attack"
            component={AttackScreen}
            options={{ title: 'CPA Attack' }}
          />
          <Stack.Screen
            name="PlaintextManager"
            component={PlaintextManagerScreen}
            options={{ title: 'Plaintext Manager' }}
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