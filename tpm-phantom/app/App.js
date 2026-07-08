/*
 * tpm-phantom — app/App.js
 * Main application entry point for the tpm-phantom companion app.
 *
 * Implements bottom-tab navigation between:
 *   - Dashboard: device status, capture controls, stats
 *   - Capture: real-time TPM transaction log
 *   - Analyze: TPM register decode and access frequency analysis
 *   - Inject: LPC and SPI TPM injection interface
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { Text, View, StyleSheet } from 'react-native';

import ble from './utils/bleManager';
import {
  RSP_STATUS, RSP_STATS, RSP_TRANSACTION, RSP_VERSION, RSP_SD_INFO,
  decodeStatus, decodeStats, decodeVersion, decodeSdInfo,
  CMD_GET_VERSION,
} from './utils/protocol';

import StatusBar from './components/StatusBar';
import ConnectScreen from './screens/ConnectScreen';
import DashboardScreen from './screens/DashboardScreen';
import CaptureScreen from './screens/CaptureScreen';
import AnalyzeScreen from './screens/AnalyzeScreen';
import InjectScreen from './screens/InjectScreen';

const Tab = createBottomTabNavigator();

const DEFAULT_STATUS = {
  mode: 0, capturing: false, totalTx: 0, tpmTx: 0,
  sdReady: false, usbConnected: false,
};

const DEFAULT_STATS = {
  lpcTotal: 0, lpcTpm: 0, spiTotal: 0,
  spiReads: 0, spiWrites: 0, spiBytes: 0,
  sdBlocks: 0, sdBytes: 0, lpcSerirq: 0,
};

export default function App() {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState(DEFAULT_STATUS);
  const [stats, setStats] = useState(DEFAULT_STATS);
  const [allTransactions, setAllTransactions] = useState([]);
  const [version, setVersion] = useState('');

  // Global BLE frame handler
  useEffect(() => {
    ble.onFrame = (cmd, payload) => {
      switch (cmd) {
        case RSP_STATUS:
          setStatus(decodeStatus(payload));
          break;
        case RSP_STATS:
          setStats(decodeStats(payload));
          break;
        case RSP_TRANSACTION: {
          // Transactions are handled per-screen; but we also keep a global list
          // for the Analyze screen
          break;
        }
        case RSP_VERSION:
          setVersion(decodeVersion(payload));
          break;
        case RSP_SD_INFO:
          // Update SD info into status
          setStatus(prev => ({ ...prev, sdReady: decodeSdInfo(payload).ready }));
          break;
        default:
          break;
      }
    };

    ble.onStatus = (type, msg) => {
      if (type === 'connected') {
        setConnected(true);
        // Request version on connect
        ble.send(CMD_GET_VERSION);
      } else if (type === 'disconnected') {
        setConnected(false);
        setStatus(DEFAULT_STATUS);
        setStats(DEFAULT_STATS);
      }
    };
  }, []);

  const handleConnected = useCallback(() => {
    setConnected(true);
  }, []);

  const handleStatusUpdate = useCallback((newStatus) => {
    setStatus(prev => ({ ...prev, ...newStatus }));
  }, []);

  const handleStatsUpdate = useCallback((newStats) => {
    setStats(prev => ({ ...prev, ...newStats }));
  }, []);

  // If not connected, show the Connect screen
  if (!connected) {
    return (
      <SafeAreaProvider>
        <ConnectScreen onConnected={handleConnected} />
      </SafeAreaProvider>
    );
  }

  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <View style={styles.container}>
          <StatusBar
            connected={connected}
            capturing={status.capturing}
            mode={status.mode}
            totalTx={status.totalTx}
            tpmTx={status.tpmTx}
          />
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: {
                backgroundColor: '#1a1a2e',
                borderTopColor: '#333',
              },
              tabBarActiveTintColor: '#00ff88',
              tabBarInactiveTintColor: '#666',
              tabBarLabelStyle: { fontFamily: 'monospace', fontSize: 11 },
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#00ff88',
              headerTitleStyle: { fontFamily: 'monospace' },
            }}>
            <Tab.Screen name="Dashboard" options={{ title: 'Dashboard' }}>
              {() => (
                <DashboardScreen
                  ble={ble}
                  status={status}
                  stats={stats}
                  onStatusUpdate={handleStatusUpdate}
                  onStatsUpdate={handleStatsUpdate}
                />
              )}
            </Tab.Screen>
            <Tab.Screen name="Capture" options={{ title: 'Capture' }}>
              {() => (
                <CaptureScreen
                  ble={ble}
                  status={status}
                  onStatusUpdate={handleStatusUpdate}
                />
              )}
            </Tab.Screen>
            <Tab.Screen name="Analyze" options={{ title: 'Analyze' }}>
              {() => <AnalyzeScreen transactions={allTransactions} />}
            </Tab.Screen>
            <Tab.Screen name="Inject" options={{ title: 'Inject' }}>
              {() => <InjectScreen ble={ble} />}
            </Tab.Screen>
          </Tab.Navigator>
        </View>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d0d1a',
  },
});