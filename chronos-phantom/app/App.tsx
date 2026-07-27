// App.tsx — Main entry point for Chronos-Phantom companion app
//
// Chronos-Phantom: IEEE 1588 PTP / NTP Time-Synchronization Attack Platform
//
// Author: jayis1
// License: GPL-2.0

import React, { useState, useEffect } from 'react';
import { SafeAreaView, StatusBar, StyleSheet, View, Text } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';

import DashboardScreen from './src/screens/DashboardScreen';
import TopologyScreen from './src/screens/TopologyScreen';
import AttackControlScreen from './src/screens/AttackControlScreen';
import BmcaSpooferScreen from './src/screens/BmcaSpooferScreen';
import CovertChannelScreen from './src/screens/CovertChannelScreen';
import CaptureLogScreen from './src/screens/CaptureLogScreen';
import SettingsScreen from './src/screens/SettingsScreen';

import { BleProvider } from './src/ble/BleManager';
import type { DeviceStatus } from './src/types';

const Tab = createBottomTabNavigator();

export default function App() {
  const [status, setStatus] = useState<DeviceStatus | null>(null);

  return (
    <BleProvider>
      <NavigationContainer>
        <SafeAreaView style={styles.container}>
          <StatusBar barStyle="light-content" backgroundColor="#0a0a0a" />
          <View style={styles.header}>
            <Text style={styles.title}>⏱ Chronos-Phantom</Text>
            <Text style={styles.subtitle}>PTP/NTP Attack Platform</Text>
          </View>
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: { backgroundColor: '#1a1a1a', borderTopColor: '#333' },
              tabBarActiveTintColor: '#00ff88',
              tabBarInactiveTintColor: '#666',
              headerShown: false,
            }}
          >
            <Tab.Screen name="Dashboard" component={DashboardScreen}
              options={{ tabBarLabel: 'Status' }} />
            <Tab.Screen name="Topology" component={TopologyScreen}
              options={{ tabBarLabel: 'Topology' }} />
            <Tab.Screen name="Attack" component={AttackControlScreen}
              options={{ tabBarLabel: 'Attack' }} />
            <Tab.Screen name="BMCA" component={BmcaSpooferScreen}
              options={{ tabBarLabel: 'BMCA' }} />
            <Tab.Screen name="Covert" component={CovertChannelScreen}
              options={{ tabBarLabel: 'Covert' }} />
            <Tab.Screen name="Capture" component={CaptureLogScreen}
              options={{ tabBarLabel: 'Capture' }} />
            <Tab.Screen name="Settings" component={SettingsScreen}
              options={{ tabBarLabel: 'Settings' }} />
          </Tab.Navigator>
        </SafeAreaView>
      </NavigationContainer>
    </BleProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  header: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
    alignItems: 'center',
  },
  title: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#00ff88',
  },
  subtitle: {
    fontSize: 11,
    color: '#666',
  },
});

// Author: jayis1
// License: GPL-2.0