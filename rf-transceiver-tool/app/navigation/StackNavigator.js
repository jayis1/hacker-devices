/**
 * StackNavigator.js — Main navigation setup
 */

import React from 'react';
import { createStackNavigator } from '@react-navigation/stack';
import DeviceScreen from '../screens/DeviceScreen';
import SettingsScreen from '../screens/SettingsScreen';

const Stack = createStackNavigator();

export default function StackNavigator() {
  return (
    <Stack.Navigator
      initialRouteName="Device"
      screenOptions={{
        headerStyle: {
          backgroundColor: '#1a1a2e',
          elevation: 0,
          shadowOpacity: 0,
        },
        headerTintColor: '#00ff88',
        headerTitleStyle: {
          fontWeight: 'bold',
        },
        cardStyle: {
          backgroundColor: '#16213e',
        },
      }}
    >
      <Stack.Screen
        name="Device"
        component={DeviceScreen}
        options={{ title: 'RF Transceiver Tool' }}
      />
      <Stack.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Configuration' }}
      />
    </Stack.Navigator>
  );
}