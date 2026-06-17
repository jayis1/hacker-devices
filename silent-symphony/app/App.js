/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * React Native App — Main Entry
 *
 * Tab-based navigation with Dashboard, Transmit, Capture, and Analyze screens.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { BleManager } from 'react-native-ble-plx';
import Icon from 'react-native-vector-icons/MaterialIcons';

import DashboardScreen from './screens/DashboardScreen';
import TransmitScreen from './screens/TransmitScreen';
import CaptureScreen from './screens/CaptureScreen';
import AnalyzeScreen from './screens/AnalyzeScreen';

const Tab = createBottomTabNavigator();

/**
 * App component — root of the SilentSymphony companion app.
 *
 * Initialises BLE manager and provides it to all screens via context.
 */
export default function App() {
  const [bleManager] = useState(() => new BleManager());
  const [connectedDevice, setConnectedDevice] = useState(null);
  const [status, setStatus] = useState(null);

  const handleDisconnect = useCallback(() => {
    if (connectedDevice) {
      connectedDevice.cancelConnection().catch(() => {});
      setConnectedDevice(null);
      setStatus(null);
    }
  }, [connectedDevice]);

  const screenOptions = {
    headerStyle: { backgroundColor: '#1a1a2e' },
    headerTintColor: '#e0e0e0',
    headerTitleStyle: { fontWeight: 'bold' },
    tabBarStyle: { backgroundColor: '#1a1a2e', borderTopColor: '#16213e' },
    tabBarActiveTintColor: '#00d4ff',
    tabBarInactiveTintColor: '#6c757d',
  };

  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <Tab.Navigator screenOptions={screenOptions}>
          <Tab.Screen
            name="Dashboard"
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="dashboard" size={size} color={color} />
              ),
            }}
          >
            {() => (
              <DashboardScreen
                bleManager={bleManager}
                connectedDevice={connectedDevice}
                setConnectedDevice={setConnectedDevice}
                status={status}
                setStatus={setStatus}
                onDisconnect={handleDisconnect}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Transmit"
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="wifi-tethering" size={size} color={color} />
              ),
            }}
          >
            {() => (
              <TransmitScreen
                bleManager={bleManager}
                connectedDevice={connectedDevice}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Capture"
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="graphic-eq" size={size} color={color} />
              ),
            }}
          >
            {() => (
              <CaptureScreen
                bleManager={bleManager}
                connectedDevice={connectedDevice}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Analyze"
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="analytics" size={size} color={color} />
              ),
            }}
          >
            {() => (
              <AnalyzeScreen
                bleManager={bleManager}
                connectedDevice={connectedDevice}
              />
            )}
          </Tab.Screen>
        </Tab.Navigator>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}