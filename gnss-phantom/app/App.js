/*
 * App.js — Main app entry point for GNSS-Phantom companion app
 *
 * Sets up React Navigation with bottom tabs, BLE connection state,
 * and global error handling.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer, DarkTheme } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';

import DashboardScreen from './screens/DashboardScreen';
import SatelliteScreen from './screens/SatelliteScreen';
import TrajectoryScreen from './screens/TrajectoryScreen';
import RFSettingsScreen from './screens/RFSettingsScreen';
import { bleManager } from './utils/ble';

const Tab = createBottomTabNavigator();

const navTheme = {
  ...DarkTheme,
  colors: {
    ...DarkTheme.colors,
    background: '#0f0f1e',
    card: '#1a1a2e',
    text: '#3b82f6',
    border: '#2a2a3e',
    primary: '#3b82f6',
  },
};

function App() {
  const [scanning, setScanning] = useState(false);
  const [foundDevice, setFoundDevice] = useState(null);

  useEffect(() => {
    return () => {
      bleManager.disconnect();
    };
  }, []);

  const startScan = () => {
    setScanning(true);
    bleManager.startScan((device) => {
      setFoundDevice(device);
      setScanning(false);
    });
  };

  const connect = async () => {
    if (foundDevice) {
      const ok = await bleManager.connect(foundDevice);
      if (ok) {
        Alert.alert('Connected', 'GNSS-Phantom connected');
      } else {
        Alert.alert('Connection Failed', 'Could not connect to device');
      }
    }
  };

  return (
    <NavigationContainer theme={navTheme}>
      <Tab.Navigator
        screenOptions={{
          tabBarStyle: { backgroundColor: '#1a1a2e', borderTopColor: '#2a2a3e' },
          tabBarActiveTintColor: '#3b82f6',
          tabBarInactiveTintColor: '#555',
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#3b82f6',
          headerTitleStyle: { fontFamily: 'monospace' },
        }}
      >
        <Tab.Screen
          name="Dashboard"
          component={DashboardScreen}
          options={{
            tabBarIcon: ({ color }) => <Text style={{ color, fontSize: 18 }}>⌂</Text>,
            title: 'GNSS-Phantom',
          }}
        />
        <Tab.Screen
          name="Satellites"
          component={SatelliteScreen}
          options={{
            tabBarIcon: ({ color }) => <Text style={{ color, fontSize: 18 }}>📡</Text>,
            title: 'Satellites',
          }}
        />
        <Tab.Screen
          name="Trajectory"
          component={TrajectoryScreen}
          options={{
            tabBarIcon: ({ color }) => <Text style={{ color, fontSize: 18 }}>🗺️</Text>,
            title: 'Trajectory',
          }}
        />
        <Tab.Screen
          name="RFSettings"
          component={RFSettingsScreen}
          options={{
            tabBarIcon: ({ color }) => <Text style={{ color, fontSize: 18 }}>📶</Text>,
            title: 'RF Settings',
          }}
        />
      </Tab.Navigator>
    </NavigationContainer>
  );
}

export default App;