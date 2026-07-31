/**
 * App.js — Main entry point for MagLance companion app
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * React Native app for controlling the MagLance magnetic field injection
 * platform via BLE 5.0. Provides real-time sensor monitoring, pulse
 * configuration, sweep control, profile management, and calibration.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DashboardScreen from './src/screens/DashboardScreen';
import PulseScreen from './src/screens/PulseScreen';
import SweepScreen from './src/screens/SweepScreen';
import SenseScreen from './src/screens/SenseScreen';
import ProfileScreen from './src/screens/ProfileScreen';
import { BLEProvider } from './src/context/BLEContext';

const Tab = createBottomTabNavigator();

/**
 * MagLance companion app — bottom tab navigation.
 * Five main screens:
 * 1. Dashboard: Real-time status, field readings, battery, temperature
 * 2. Pulse: Configure and fire single/multi-pulse sequences
 * 3. Sweep: Frequency sweep configuration with live spectrum
 * 4. Sense: Passive magnetic field visualization and logging
 * 5. Profile: Create, edit, save, load attack profiles
 */
export default function App() {
  return (
    <SafeAreaProvider>
      <BLEProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarActiveTintColor: '#e74c3c',
              tabBarInactiveTintColor: '#95a5a6',
              tabBarStyle: { backgroundColor: '#1a1a2e' },
              headerStyle: { backgroundColor: '#16213e' },
              headerTintColor: '#ffffff',
            }}
          >
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{
                tabBarLabel: 'Status',
                tabBarIcon: ({ color, size }) => (
                  <Icon name="gauge" color={color} size={size} />
                ),
              }}
            />
            <Tab.Screen
              name="Pulse"
              component={PulseScreen}
              options={{
                tabBarLabel: 'Pulse',
                tabBarIcon: ({ color, size }) => (
                  <Icon name="waveform" color={color} size={size} />
                ),
              }}
            />
            <Tab.Screen
              name="Sweep"
              component={SweepScreen}
              options={{
                tabBarLabel: 'Sweep',
                tabBarIcon: ({ color, size }) => (
                  <Icon name="sine-wave" color={color} size={size} />
                ),
              }}
            />
            <Tab.Screen
              name="Sense"
              component={SenseScreen}
              options={{
                tabBarLabel: 'Sense',
                tabBarIcon: ({ color, size }) => (
                  <Icon name="radar" color={color} size={size} />
                ),
              }}
            />
            <Tab.Screen
              name="Profile"
              component={ProfileScreen}
              options={{
                tabBarLabel: 'Profiles',
                tabBarIcon: ({ color, size }) => (
                  <Icon name="content-save" color={color} size={size} />
                ),
              }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </BLEProvider>
    </SafeAreaProvider>
  );
}