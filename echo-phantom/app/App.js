/**
 * App.js — ECHO-Phantom Companion App
 *
 * Author: jayis1
 * License: MIT
 *
 * React Native app for controlling the ECHO-Phantom I²S/TDM audio
 * bus MITM implant over encrypted BLE 5.0.
 *
 * Screens:
 *   - Dashboard: device status, battery, mode, format
 *   - Capture: start/stop audio capture, live waveform
 *   - Inject: upload/manage inject clips, start/stop injection
 *   - Modify: configure real-time DSP filters
 *   - Stream: live audio monitoring
 *   - Settings: BLE pairing, firmware update, factory reset
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { Provider as PaperProvider, Text, Button } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialIcons';

import DashboardScreen from './screens/DashboardScreen';
import CaptureScreen from './screens/CaptureScreen';
import InjectScreen from './screens/InjectScreen';
import ModifyScreen from './screens/ModifyScreen';
import StreamScreen from './screens/StreamScreen';
import SettingsScreen from './screens/SettingsScreen';
import { BLEContext, BLEProvider } from './components/BLEManager';

const Tab = createBottomTabNavigator();

const SCREENS = {
  Dashboard: { component: DashboardScreen, icon: 'dashboard' },
  Capture: { component: CaptureScreen, icon: 'mic' },
  Inject: { component: InjectScreen, icon: 'graphic-eq' },
  Modify: { component: ModifyScreen, icon: 'tune' },
  Stream: { component: StreamScreen, icon: 'hearing' },
  Settings: { component: SettingsScreen, icon: 'settings' },
};

function App() {
  return (
    <PaperProvider>
      <BLEProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={({ route }) => ({
              tabBarIcon: ({ color, size }) => {
                const iconName = SCREENS[route.name]?.icon || 'help';
                return <Icon name={iconName} size={size} color={color} />;
              },
              tabBarActiveTintColor: '#e91e63',
              tabBarInactiveTintColor: '#888',
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#fff',
              headerTitleStyle: { fontWeight: 'bold' },
            })}
          >
            <Tab.Screen name="Dashboard" component={DashboardScreen} />
            <Tab.Screen name="Capture" component={CaptureScreen} />
            <Tab.Screen name="Inject" component={InjectScreen} />
            <Tab.Screen name="Modify" component={ModifyScreen} />
            <Tab.Screen name="Stream" component={StreamScreen} />
            <Tab.Screen name="Settings" component={SettingsScreen} />
          </Tab.Navigator>
        </NavigationContainer>
      </BLEProvider>
    </PaperProvider>
  );
}

export default App;