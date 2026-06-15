/**
 * App.js — USB DMA Phantom Companion App
 * React Native navigation shell with BLE C2 interface
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { Provider as PaperProvider, DarkTheme } from 'react-native-paper';

// Screens
import HomeScreen from './screens/HomeScreen';
import DmaControlScreen from './screens/DmaControlScreen';
import PayloadManagerScreen from './screens/PayloadManagerScreen';
import SettingsScreen from './screens/SettingsScreen';

// BLE Manager
import { BleManagerProvider } from './utils/protocol';

const Stack = createNativeStackNavigator();

const darkTheme = {
  ...DarkTheme,
  colors: {
    ...DarkTheme.colors,
    primary: '#00ff88',
    accent: '#ff3366',
    background: '#0a0a0a',
    surface: '#1a1a2e',
    text: '#e0e0e0',
    card: '#16213e',
    border: '#2a2a4a',
  },
};

export default function App() {
  return (
    <PaperProvider theme={darkTheme}>
      <BleManagerProvider>
        <NavigationContainer>
          <Stack.Navigator
            initialRouteName="Home"
            screenOptions={{
              headerStyle: { backgroundColor: '#0a0a0a' },
              headerTintColor: '#00ff88',
              headerTitleStyle: { fontWeight: 'bold' },
              contentStyle: { backgroundColor: '#0a0a0a' },
            }}
          >
            <Stack.Screen
              name="Home"
              component={HomeScreen}
              options={{ title: 'DMA Phantom' }}
            />
            <Stack.Screen
              name="DmaControl"
              component={DmaControlScreen}
              options={{ title: 'DMA Control' }}
            />
            <Stack.Screen
              name="PayloadManager"
              component={PayloadManagerScreen}
              options={{ title: 'Payload Manager' }}
            />
            <Stack.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Stack.Navigator>
        </NavigationContainer>
      </BleManagerProvider>
    </PaperProvider>
  );
}