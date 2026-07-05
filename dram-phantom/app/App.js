/**
 * App.js — DRAM-Phantom companion app root
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Provides a bottom-tab navigator with five screens:
 *   Snoop | Rowhammer | WarmBoot | CovertChannel | Settings
 * The transport (BLE or USB CDC) is abstracted by DeviceContext so the same
 * screens work over either link.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { DeviceProvider } from './components/DeviceContext';
import SnoopScreen from './screens/SnoopScreen';
import RowhammerScreen from './screens/RowhammerScreen';
import WarmBootScreen from './screens/WarmBootScreen';
import CovertChannelScreen from './screens/CovertChannelScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            headerStyle: { backgroundColor: '#0d1117' },
            headerTintColor: '#e6edf3',
            tabBarStyle: { backgroundColor: '#0d1117' },
            tabBarActiveTintColor: '#58a6ff',
            tabBarInactiveTintColor: '#7d8590',
          }}
        >
          <Tab.Screen name="Snoop" component={SnoopScreen} />
          <Tab.Screen name="Rowhammer" component={RowhammerScreen} />
          <Tab.Screen name="WarmBoot" component={WarmBootScreen} />
          <Tab.Screen name="Covert" component={CovertChannelScreen} />
          <Tab.Screen name="Settings" component={SettingsScreen} />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}