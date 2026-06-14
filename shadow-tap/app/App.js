/**
 * App.js — ShadowTap Companion App
 * React Native app for controlling the ShadowTap PoE Network Tap
 * via BLE connection to the nRF52840-M.2 module.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// Screens
import DashboardScreen from './screens/DashboardScreen';
import RulesScreen from './screens/RulesScreen';
import CaptureScreen from './screens/CaptureScreen';

// BLE Protocol utilities
import { ShadowTapBLE } from './utils/protocol';

const Tab = createBottomTabNavigator();

export default function App() {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState({
    uplinkLink: false,
    targetLink: false,
    mode: 'tap',
    capturing: false,
    ruleCount: 0,
  });

  // BLE connection manager
  const bleManager = new ShadowTapBLE();

  useEffect(() => {
    // Attempt auto-connect on mount
    bleManager.autoConnect((connectedDevice) => {
      setDevice(connectedDevice);
      setConnected(true);
      // Start polling status
      const interval = setInterval(async () => {
        const s = await bleManager.getStatus();
        if (s) setStatus(s);
      }, 2000);
      return () => clearInterval(interval);
    });

    return () => {
      bleManager.disconnect();
    };
  }, []);

  const handleConnect = async () => {
    const dev = await bleManager.scanAndConnect();
    if (dev) {
      setDevice(dev);
      setConnected(true);
    }
  };

  const handleDisconnect = async () => {
    await bleManager.disconnect();
    setDevice(null);
    setConnected(false);
  };

  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={({ route }) => ({
          tabBarIcon: ({ color, size }) => {
            let iconName;
            if (route.name === 'Dashboard') {
              iconName = 'view-dashboard';
            } else if (route.name === 'Rules') {
              iconName = 'filter-settings';
            } else if (route.name === 'Capture') {
              iconName = 'archive';
            }
            return <Icon name={iconName} size={size} color={color} />;
          },
          tabBarActiveTintColor: '#00ff88',
          tabBarInactiveTintColor: '#666',
          tabBarStyle: { backgroundColor: '#1a1a2e' },
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#00ff88',
        })}
      >
        <Tab.Screen name="Dashboard">
          {(props) => (
            <DashboardScreen
              {...props}
              connected={connected}
              status={status}
              onConnect={handleConnect}
              onDisconnect={handleDisconnect}
              bleManager={bleManager}
            />
          )}
        </Tab.Screen>
        <Tab.Screen name="Rules">
          {(props) => (
            <RulesScreen
              {...props}
              connected={connected}
              bleManager={bleManager}
            />
          )}
        </Tab.Screen>
        <Tab.Screen name="Capture">
          {(props) => (
            <CaptureScreen
              {...props}
              connected={connected}
              bleManager={bleManager}
            />
          )}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}