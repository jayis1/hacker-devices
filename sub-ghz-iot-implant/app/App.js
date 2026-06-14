/**
 * App.js — Substation Companion App
 * Main entry point with navigation for the Sub-GHz IoT Gateway Implant
 *
 * Screens:
 *   - DeviceScreen: Connection status, device info
 *   - CaptureScreen: Live packet capture & monitoring
 *   - SettingsScreen: Radio configuration, frequency, power
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DeviceScreen from './screens/DeviceScreen';
import CaptureScreen from './screens/CaptureScreen';
import SettingsScreen from './screens/SettingsScreen';
import { BleProvider } from './components/BleContext';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <BleProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarActiveTintColor: '#00E676',
            tabBarInactiveTintColor: '#757575',
            tabBarStyle: {
              backgroundColor: '#1A1A2E',
              borderTopColor: '#16213E',
            },
            headerStyle: {
              backgroundColor: '#1A1A2E',
            },
            headerTintColor: '#FFFFFF',
          }}
        >
          <Tab.Screen
            name="Device"
            component={DeviceScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="access-point" color={color} size={size} />
              ),
              tabBarLabel: 'Device',
            }}
          />
          <Tab.Screen
            name="Capture"
            component={CaptureScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="access-point-network" color={color} size={size} />
              ),
              tabBarLabel: 'Capture',
            }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="cog" color={color} size={size} />
              ),
              tabBarLabel: 'Settings',
            }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </BleProvider>
  );
}