/**
 * App.js — eMMC Flash Dumper Companion App
 *
 * React Native application with bottom-tab navigation for controlling
 * the eMMC Flash Dumper hardware device over USB. Provides acquisition
 * control, real-time progress monitoring, hex data viewing, and
 * SHA-256 hash verification.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  NavigationContainer,
  useNavigationContainerRef,
} from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import {
  StatusBar,
  Text,
  View,
  StyleSheet,
  AppState,
  Alert,
} from 'react-native';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import AcquisitionScreen from './screens/AcquisitionScreen';
import AnalysisScreen from './screens/AnalysisScreen';
import DeviceInfoScreen from './screens/DeviceInfoScreen';
import SettingsScreen from './screens/SettingsScreen';

import { FlashDumperProtocol } from './utils/protocol';
import { DeviceContext, DeviceProvider } from './components/DeviceContext';

const Tab = createBottomTabNavigator();

/*===========================================================================
 * Tab Bar Icon Component (simple text-based icons)
 *===========================================================================*/

function TabIcon({ label, focused, color }) {
  const icons = {
    Acquire: focused ? '⬇' : '⬇',
    Analyze: focused ? '🔍' : '🔍',
    Device: focused ? '📟' : '📟',
    Settings: focused ? '⚙' : '⚙',
  };
  return (
    <View style={styles.tabIconContainer}>
      <Text style={[styles.tabIcon, { color }]}>{icons[label] || '●'}</Text>
    </View>
  );
}

/*===========================================================================
 * Main App Component
 *===========================================================================*/

export default function App() {
  const navigationRef = useNavigationContainerRef();
  const [deviceConnected, setDeviceConnected] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [batteryMv, setBatteryMv] = useState(0);
  const protocolRef = useRef(null);

  /* Initialize USB protocol handler */
  useEffect(() => {
    protocolRef.current = new FlashDumperProtocol();

    const onDeviceAttached = (info) => {
      setDeviceConnected(true);
      setDeviceInfo(info);
      Alert.alert('Device Connected', `eMMC Flash Dumper detected`);
    };

    const onDeviceDetached = () => {
      setDeviceConnected(false);
      setDeviceInfo(null);
      Alert.alert('Device Disconnected', 'eMMC Flash Dumper disconnected');
    };

    const onBatteryUpdate = (mv) => {
      setBatteryMv(mv);
    };

    protocolRef.current.on('attached', onDeviceAttached);
    protocolRef.current.on('detached', onDeviceDetached);
    protocolRef.current.on('battery', onBatteryUpdate);

    /* Start device discovery */
    protocolRef.current.startDiscovery();

    return () => {
      protocolRef.current?.stopDiscovery();
      protocolRef.current?.removeAllListeners();
    };
  }, []);

  /* Handle app state changes (background/foreground) */
  useEffect(() => {
    const handleAppState = (nextState) => {
      if (nextState === 'active') {
        protocolRef.current?.startDiscovery();
      } else {
        protocolRef.current?.stopDiscovery();
      }
    };
    const subscription = AppState.addEventListener('change', handleAppState);
    return () => subscription.remove();
  }, []);

  /* Device context value */
  const deviceContextValue = {
    connected: deviceConnected,
    deviceInfo: deviceInfo,
    batteryMv: batteryMv,
    protocol: protocolRef.current,
  };

  return (
    <SafeAreaProvider>
      <DeviceProvider value={deviceContextValue}>
        <NavigationContainer ref={navigationRef}>
          <StatusBar barStyle="light-content" backgroundColor="#0a0a0a" />
          <Tab.Navigator
            screenOptions={({ route }) => ({
              headerStyle: {
                backgroundColor: '#0a0a0a',
                borderBottomColor: '#1a1a1a',
                borderBottomWidth: 1,
              },
              headerTintColor: '#00ff88',
              headerTitleStyle: {
                fontFamily: 'monospace',
                fontSize: 16,
                fontWeight: 'bold',
              },
              tabBarStyle: {
                backgroundColor: '#0a0a0a',
                borderTopColor: '#1a1a1a',
                borderTopWidth: 1,
                height: 60,
                paddingBottom: 8,
              },
              tabBarActiveTintColor: '#00ff88',
              tabBarInactiveTintColor: '#555555',
              tabBarLabelStyle: {
                fontFamily: 'monospace',
                fontSize: 10,
              },
              tabBarIcon: ({ focused, color }) => (
                <TabIcon label={route.name} focused={focused} color={color} />
              ),
            })}
          >
            <Tab.Screen
              name="Acquire"
              component={AcquisitionScreen}
              options={{
                title: 'ACQUIRE',
                headerRight: () => (
                  <View style={styles.headerRight}>
                    <Text style={styles.headerStatus}>
                      {deviceConnected ? '● LIVE' : '○ OFFLINE'}
                    </Text>
                    {deviceConnected && (
                      <Text style={styles.headerBattery}>
                        {batteryMv > 0
                          ? `${(batteryMv / 1000).toFixed(2)}V`
                          : ''}
                      </Text>
                    )}
                  </View>
                ),
              }}
            />
            <Tab.Screen
              name="Analyze"
              component={AnalysisScreen}
              options={{ title: 'ANALYZE' }}
            />
            <Tab.Screen
              name="Device"
              component={DeviceInfoScreen}
              options={{ title: 'DEVICE' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'SETTINGS' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </DeviceProvider>
    </SafeAreaProvider>
  );
}

/*===========================================================================
 * Styles
 *===========================================================================*/

const styles = StyleSheet.create({
  tabIconContainer: {
    alignItems: 'center',
    justifyContent: 'center',
    width: 28,
    height: 28,
  },
  tabIcon: {
    fontSize: 20,
  },
  headerRight: {
    flexDirection: 'row',
    alignItems: 'center',
    marginRight: 12,
    gap: 8,
  },
  headerStatus: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#00ff88',
  },
  headerBattery: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#888888',
  },
});
