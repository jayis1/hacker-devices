/**
 * App.js — WireReaper companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Main navigation and app structure for the WireReaper multi-bus
 * infiltrator companion app. Provides BLE connection management
 * and navigation to all bus tool screens.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import { DeviceProvider } from './components/DeviceContext';
import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import CaptureViewScreen from './screens/CaptureViewScreen';
import I2cConsoleScreen from './screens/I2cConsoleScreen';
import SpiConsoleScreen from './screens/SpiConsoleScreen';
import UartTerminalScreen from './screens/UartTerminalScreen';
import OneWireScreen from './screens/OneWireScreen';
import ReplayManagerScreen from './screens/ReplayManagerScreen';
import FuzzerScreen from './screens/FuzzerScreen';

const Stack = createStackNavigator();

/**
 * Root component for the WireReaper app.
 * Author: jayis1
 */
export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Connection"
          screenOptions={{
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#e94560',
            headerTitleStyle: { fontWeight: 'bold' },
          }}
        >
          <Stack.Screen
            name="Connection"
            component={ConnectionScreen}
            options={{ title: 'WireReaper — by jayis1' }}
          />
          <Stack.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'Dashboard' }}
          />
          <Stack.Screen
            name="CaptureView"
            component={CaptureViewScreen}
            options={{ title: 'Live Capture' }}
          />
          <Stack.Screen
            name="I2cConsole"
            component={I2cConsoleScreen}
            options={{ title: 'I²C Console' }}
          />
          <Stack.Screen
            name="SpiConsole"
            component={SpiConsoleScreen}
            options={{ title: 'SPI Console' }}
          />
          <Stack.Screen
            name="UartTerminal"
            component={UartTerminalScreen}
            options={{ title: 'UART Terminal' }}
          />
          <Stack.Screen
            name="OneWire"
            component={OneWireScreen}
            options={{ title: '1-Wire Tools' }}
          />
          <Stack.Screen
            name="ReplayManager"
            component={ReplayManagerScreen}
            options={{ title: 'Replay Manager' }}
          />
          <Stack.Screen
            name="Fuzzer"
            component={FuzzerScreen}
            options={{ title: 'Bus Fuzzer' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}