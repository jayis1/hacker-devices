/**
 * App.js — Joule-Phantom companion app entry point.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * React Native app that connects to the Joule-Phantom smart-battery
 * MITM implant over BLE.  Provides:
 *   - Dashboard with live battery telemetry (real vs. spoofed)
 *   - MITM rule editor (spoof / block / glitch / inject)
 *   - Spoof profile quick-launcher (full-charge, empty, overtemp, etc.)
 *   - Capture viewer (streamed SMBus frame log)
 *   - Inject console (arbitrary SBS command injection)
 *
 * Legal / ethical disclaimer:
 *   This app and the associated hardware are designed EXCLUSIVELY for
 *   authorized security research, penetration testing with explicit
 *   written consent, and red-team operations on systems you own or
 *   have explicit permission to assess.  Unauthorized interception or
 *   manipulation of battery telemetry on devices you do not own may
 *   violate computer-fraud, wiretap, and product-safety laws.  The
 *   author (jayis1) assumes no liability for misuse.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { View, Text, StyleSheet, Alert } from 'react-native';

import BleService from './services/BleService';
import DashboardScreen from './screens/DashboardScreen';
import MitmScreen from './screens/MitmScreen';
import CaptureScreen from './screens/CaptureScreen';
import InjectScreen from './screens/InjectScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const TX_CHAR_UUID   = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write
const RX_CHAR_UUID   = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify

export default function App() {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState({});
  const [frames, setFrames] = useState([]);
  const [rules, setRules] = useState([]);

  // BLE connection lifecycle
  useEffect(() => {
    BleService.initialize(SERVICE_UUID, TX_CHAR_UUID, RX_CHAR_UUID);

    const unsubStatus = BleService.onStatus((s) => {
      setStatus(s);
      setConnected(true);
    });
    const unsubFrame = BleService.onFrame((f) => {
      setFrames((prev) => [...prev.slice(-499), f]);
    });
    const unsubRules = BleService.onRules((r) => setRules(r));
    const unsubConn = BleService.onConnection((c) => setConnected(c));

    // Auto-connect attempt
    BleService.connect().catch((e) =>
      console.log('BLE connect deferred:', e)
    );

    return () => {
      unsubStatus();
      unsubFrame();
      unsubRules();
      unsubConn();
      BleService.disconnect();
    };
  }, []);

  const sendCommand = useCallback((op, payload) => {
    return BleService.sendCommand(op, payload);
  }, []);

  return (
    <NavigationContainer>
      <View style={styles.header}>
        <Text style={styles.title}>⚡ Joule-Phantom</Text>
        <View style={[styles.dot, { backgroundColor: connected ? '#0f0' : '#f00' }]} />
        <Text style={styles.conn}>{connected ? 'Connected' : 'Scanning…'}</Text>
      </View>
      <Tab.Navigator
        screenOptions={{
          headerShown: false,
          tabBarStyle: { backgroundColor: '#1a1a2e', borderTopColor: '#333' },
          tabBarActiveTintColor: '#e94560',
          tabBarInactiveTintColor: '#888',
        }}
      >
        <Tab.Screen name="Dashboard">
          {() => <DashboardScreen status={status} connected={connected} />}
        </Tab.Screen>
        <Tab.Screen name="MITM Rules">
          {() => <MitmScreen rules={rules} sendCommand={sendCommand} />}
        </Tab.Screen>
        <Tab.Screen name="Capture">
          {() => <CaptureScreen frames={frames} />}
        </Tab.Screen>
        <Tab.Screen name="Inject">
          {() => <InjectScreen sendCommand={sendCommand} />}
        </Tab.Screen>
        <Tab.Screen name="Settings">
          {() => <SettingsScreen sendCommand={sendCommand} status={status} />}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 10,
    backgroundColor: '#1a1a2e',
    paddingTop: 40,
  },
  title: {
    color: '#e94560',
    fontSize: 18,
    fontWeight: 'bold',
    flex: 1,
  },
  dot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 6,
  },
  conn: {
    color: '#aaa',
    fontSize: 12,
  },
});