/**
 * TRACE-REAPER — Companion app entry point
 *
 * Bottom-tab navigation between Dashboard, Config, LiveTrace, Results, and
 * Storage screens. Talks to the device over BLE using utils/protocol.js.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import DashboardScreen from './screens/DashboardScreen';
import ConfigScreen from './screens/ConfigScreen';
import LiveTraceScreen from './screens/LiveTraceScreen';
import ResultsScreen from './screens/ResultsScreen';
import StorageScreen from './screens/StorageScreen';

import { DeviceConnection, ConnectionContext } from './utils/protocol';

const Tab = createBottomTabNavigator();

export default function App() {
  const [conn] = useState(() => new DeviceConnection());

  useEffect(() => {
    /* Auto-connect on mount; the operator can also use the Dashboard
     * Connect button. BLE scan + connect is handled in protocol.js. */
    conn.startScan().catch(() => {});
    return () => { conn.disconnect(); };
  }, [conn]);

  return (
    <SafeAreaProvider>
      <ConnectionContext.Provider value={conn}>
        <NavigationContainer>
          <Tab.Navigator initialRouteName="Dashboard">
            <Tab.Screen name="Dashboard"  component={DashboardScreen} />
            <Tab.Screen name="Config"      component={ConfigScreen} />
            <Tab.Screen name="Live Trace"  component={LiveTraceScreen} />
            <Tab.Screen name="Results"     component={ResultsScreen} />
            <Tab.Screen name="Sessions"     component={StorageScreen} />
          </Tab.Navigator>
        </NavigationContainer>
      </ConnectionContext.Provider>
    </SafeAreaProvider>
  );
}