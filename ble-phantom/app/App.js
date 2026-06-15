/**
 * App.js — BLE Phantom Companion App Entry Point
 *
 * Provides navigation between the main screens:
 * - Dashboard: Device status, mode selection, quick actions
 * - Sniffer: Real-time BLE packet capture and display
 * - MITM: Man-in-the-middle relay configuration
 * - Settings: Device configuration, firmware update
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import DashboardScreen from './screens/DashboardScreen';
import SnifferScreen from './screens/SnifferScreen';
import MITMScreen from './screens/MITMScreen';
import SettingsScreen from './screens/SettingsScreen';
import { ConnectionProvider } from './components/ConnectionContext';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <ConnectionProvider>
      <NavigationContainer>
        <Tab.Navigator
          initialRouteName="Dashboard"
          screenOptions={{
            tabBarActiveTintColor: '#00FF88',
            tabBarInactiveTintColor: '#666666',
            tabBarStyle: {
              backgroundColor: '#1A1A2E',
              borderTopColor: '#333333',
            },
            headerStyle: {
              backgroundColor: '#1A1A2E',
            },
            headerTintColor: '#00FF88',
          }}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{
              tabBarLabel: 'Home',
              tabBarIcon: ({ color, size }) => null, // Icon handled by vector-icons
            }}
          />
          <Tab.Screen
            name="Sniffer"
            component={SnifferScreen}
            options={{
              tabBarLabel: 'Sniffer',
            }}
          />
          <Tab.Screen
            name="MITM"
            component={MITMScreen}
            options={{
              tabBarLabel: 'MITM',
            }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{
              tabBarLabel: 'Settings',
            }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </ConnectionProvider>
  );
}