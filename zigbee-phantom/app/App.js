// App.js — ZIGBEE-PHANTOM companion app entry point
// Author: jayis1
// License: MIT
//
// React Native companion app for the ZIGBEE-PHANTOM hardware device.
// Connects over encrypted BLE 5.0, provides real-time frame capture view,
// mesh topology graph, key recovery, injection controls, and settings.

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialIcons';

import DashboardScreen from './src/screens/DashboardScreen';
import CaptureScreen from './src/screens/CaptureScreen';
import TopologyScreen from './src/screens/TopologyScreen';
import KeyRecoveryScreen from './src/screens/KeyRecoveryScreen';
import InjectionScreen from './src/screens/InjectionScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import { DeviceProvider } from './src/context/DeviceContext';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ color, size }) => {
              const icons = {
                Dashboard: 'dashboard',
                Capture: 'wifi-tethering',
                Topology: 'hub',
                Keys: 'vpn-key',
                Inject: 'flash-on',
                Settings: 'settings',
              };
              return <Icon name={icons[route.name] || 'help'} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#e85d2c',
            tabBarInactiveTintColor: '#666',
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#fff',
          })}
        >
          <Tab.Screen name="Dashboard" component={DashboardScreen} options={{ title: 'ZIGBEE-PHANTOM' }} />
          <Tab.Screen name="Capture" component={CaptureScreen} options={{ title: 'Live Capture' }} />
          <Tab.Screen name="Topology" component={TopologyScreen} options={{ title: 'Mesh Map' }} />
          <Tab.Screen name="Keys" component={KeyRecoveryScreen} options={{ title: 'Key Recovery' }} />
          <Tab.Screen name="Inject" component={InjectionScreen} options={{ title: 'Injection' }} />
          <Tab.Screen name="Settings" component={SettingsScreen} options={{ title: 'Settings' }} />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}