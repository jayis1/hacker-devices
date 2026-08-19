/**
 * App.js — Harmonic Reaper companion app (React Native)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Entry point. Sets up navigation between the 6 screens and initializes
 * the BLE manager singleton.
 */

import React, { useEffect, useState } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StyleSheet, Text } from 'react-native';

import BLEManager from './components/BLEManager';
import DashboardScreen from './screens/DashboardScreen';
import SweepScreen from './screens/SweepScreen';
import ThresholdScreen from './screens/ThresholdScreen';
import HitLogScreen from './screens/HitLogScreen';
import SettingsScreen from './screens/SettingsScreen';
import CalibrationScreen from './screens/CalibrationScreen';

const Tab = createBottomTabNavigator();

const ble = new BLEManager();

export default function App() {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState(null);
  const [hits, setHits] = useState([]);

  useEffect(() => {
    ble.onStatus = (s) => setStatus(s);
    ble.onConnected = (c) => setConnected(c);
    ble.onHit = (h) => setHits((prev) => [...prev, h]);
    return () => ble.disconnect();
  }, []);

  if (!connected) {
    return (
      <SafeAreaView style={styles.connectContainer}>
        <Text style={styles.title}>Harmonic Reaper</Text>
        <Text style={styles.subtitle}>Non-Linear Junction Detector</Text>
        <Text style={styles.author}>by jayis1 — GPL-2.0</Text>
        <Text style={styles.disclaimer}>
          ⚖️ For authorized TSCM / security survey use only. Operate in
          accordance with local RF and surveillance regulations.
        </Text>
        <Text style={styles.status}>Scanning for HR-NLJD device…</Text>
        <Text style={styles.hint}>Power on the wand and wait for BLE pairing.</Text>
      </SafeAreaView>
    );
  }

  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={{
          tabBarStyle: { backgroundColor: '#111' },
          tabBarActiveTintColor: '#00ff88',
          tabBarInactiveTintColor: '#666',
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#fff',
        }}
      >
        <Tab.Screen name="Dashboard" options={{ title: 'Live' }}>
          {(props) => <DashboardScreen {...props} ble={ble} status={status} />}
        </Tab.Screen>
        <Tab.Screen name="Sweep" options={{ title: 'Sweep Map' }}>
          {(props) => <SweepScreen {...props} hits={hits} />}
        </Tab.Screen>
        <Tab.Screen name="Threshold" options={{ title: 'Settings' }}>
          {(props) => <ThresholdScreen {...props} ble={ble} />}
        </Tab.Screen>
        <Tab.Screen name="Hits" options={{ title: 'Hit Log' }}>
          {(props) => <HitLogScreen {...props} hits={hits} />}
        </Tab.Screen>
        <Tab.Screen name="Calibrate" options={{ title: 'Calibrate' }}>
          {(props) => <CalibrationScreen {...props} ble={ble} />}
        </Tab.Screen>
        <Tab.Screen name="Settings" options={{ title: 'System' }}>
          {(props) => <SettingsScreen {...props} ble={ble} />}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  connectContainer: {
    flex: 1,
    backgroundColor: '#0a0a0a',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 20,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#00ff88',
    marginBottom: 4,
  },
  subtitle: {
    fontSize: 16,
    color: '#888',
    marginBottom: 8,
  },
  author: {
    fontSize: 12,
    color: '#555',
    marginBottom: 24,
  },
  disclaimer: {
    fontSize: 11,
    color: '#ff6666',
    textAlign: 'center',
    marginBottom: 24,
    lineHeight: 16,
  },
  status: {
    fontSize: 14,
    color: '#ccc',
    marginBottom: 4,
  },
  hint: {
    fontSize: 12,
    color: '#666',
  },
});