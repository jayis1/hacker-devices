/*
 * App.js — CoilGrip companion app root
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Tab navigator across the seven CoilGrip screens.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { Provider as PaperProvider } from 'react-native-paper';
import { DeviceProvider } from './components/DeviceContext';

import DashboardScreen from './screens/DashboardScreen';
import MitmControlScreen from './screens/MitmControlScreen';
import PowerProfilerScreen from './screens/PowerProfilerScreen';
import GlitchControlScreen from './screens/GlitchControlScreen';
import CovertChannelScreen from './screens/CovertChannelScreen';
import SnifferScreen from './screens/SnifferScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <PaperProvider>
      <DeviceProvider>
        <NavigationContainer>
          <Tab.Navigator initialRouteName="Dashboard">
            <Tab.Screen name="Dashboard"   component={DashboardScreen}   options={{ title: 'Dashboard' }} />
            <Tab.Screen name="MITM"        component={MitmControlScreen}   options={{ title: 'MITM' }} />
            <Tab.Screen name="Profiler"    component={PowerProfilerScreen} options={{ title: 'Profiler' }} />
            <Tab.Screen name="Glitch"      component={GlitchControlScreen} options={{ title: 'Glitch' }} />
            <Tab.Screen name="Exfil"       component={CovertChannelScreen}  options={{ title: 'Exfil' }} />
            <Tab.Screen name="Sniffer"     component={SnifferScreen}        options={{ title: 'Sniffer' }} />
            <Tab.Screen name="Settings"    component={SettingsScreen}      options={{ title: 'Settings' }} />
          </Tab.Navigator>
        </NavigationContainer>
      </DeviceProvider>
    </PaperProvider>
  );
}