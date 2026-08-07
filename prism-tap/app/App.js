/**
 * App.js — Prism-Tap Companion App Entry Point
 * MIPI CSI-2 / DSI Camera & Display Interface Tap & Injection Implant
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This is the main entry point for the Prism-Tap React Native companion app.
 * It sets up navigation between the Connect, Capture, Inject, Gallery, and
 * Settings screens, and provides the BLE device context to all screens.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { DeviceProvider } from './utils/deviceContext';
import ConnectScreen from './screens/ConnectScreen';
import CaptureScreen from './screens/CaptureScreen';
import InjectScreen from './screens/InjectScreen';
import GalleryScreen from './screens/GalleryScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createStackNavigator();

const App = () => {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Connect"
          screenOptions={{
            headerStyle: {
              backgroundColor: '#1a1a2e',
            },
            headerTintColor: '#00d9ff',
            headerTitleStyle: {
              fontWeight: 'bold',
            },
          }}
        >
          <Stack.Screen
            name="Connect"
            component={ConnectScreen}
            options={{ title: 'Prism-Tap — Connect' }}
          />
          <Stack.Screen
            name="Capture"
            component={CaptureScreen}
            options={{ title: 'Prism-Tap — Capture' }}
          />
          <Stack.Screen
            name="Inject"
            component={InjectScreen}
            options={{ title: 'Prism-Tap — Inject' }}
          />
          <Stack.Screen
            name="Gallery"
            component={GalleryScreen}
            options={{ title: 'Prism-Tap — Gallery' }}
          />
          <Stack.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Prism-Tap — Settings' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
};

export default App;