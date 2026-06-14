/**
 * PHANTOM — React Native App Entry Point
 * USB HID Emulation Payload Injector Control Interface
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under MIT
 *
 * WARNING: This app is for authorized security research only.
 * Unauthorized access to computer systems is illegal.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { BleProvider } from './components/BleContext';
import DeviceScreen from './screens/DeviceScreen';
import PayloadScreen from './screens/PayloadScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createStackNavigator();

function App() {
  return (
    <BleProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Device"
          screenOptions={{
            headerStyle: {
              backgroundColor: '#1a1a2e',
            },
            headerTintColor: '#00ff41',
            headerTitleStyle: {
              fontWeight: 'bold',
              fontFamily: 'monospace',
            },
            cardStyle: {
              backgroundColor: '#0f0f23',
            },
          }}
        >
          <Stack.Screen
            name="Device"
            component={DeviceScreen}
            options={{ title: 'PHANTOM' }}
          />
          <Stack.Screen
            name="Payloads"
            component={PayloadScreen}
            options={{ title: 'Payload Manager' }}
          />
          <Stack.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Configuration' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </BleProvider>
  );
}

export default App;