/**
 * App.js — 1553-Phantom companion app entry
 *
 * Author: jayis1
 * License: MIT
 *
 * Bottom-tab navigator with four screens:
 *   1. Dashboard       — role selector, channel activity, armed state, battery
 *   2. BusMonitor       — live decoded 1553 word stream + fault counters
 *   3. ScheduleEditor   — BC minor/major frame CSV editor + templates
 *   4. RtMitmConsole    — RT substore editor + MITM match/replace rules
 *
 * All screens share a single CDC connection from utils/cdc.js.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';

import Dashboard from './screens/Dashboard';
import BusMonitor from './screens/BusMonitor';
import ScheduleEditor from './screens/ScheduleEditor';
import RtMitmConsole from './screens/RtMitmConsole';
import { CdcProvider } from './utils/cdc';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <SafeAreaProvider>
      <CdcProvider>
        <NavigationContainer>
          <StatusBar style="light" />
          <Tab.Navigator
            screenOptions={{
              headerStyle: { backgroundColor: '#0d1117' },
              headerTintColor: '#c9d1d9',
              tabBarStyle: { backgroundColor: '#0d1117', borderTopColor: '#30363d' },
              tabBarActiveTintColor: '#58a6ff',
              tabBarInactiveTintColor: '#8b949e',
            }}
          >
            <Tab.Screen
              name="Dashboard"
              component={Dashboard}
              options={{ title: '1553-Phantom' }}
            />
            <Tab.Screen name="Monitor" component={BusMonitor} />
            <Tab.Screen name="Schedule" component={ScheduleEditor} />
            <Tab.Screen name="RT / MITM" component={RtMitmConsole} />
          </Tab.Navigator>
        </NavigationContainer>
      </CdcProvider>
    </SafeAreaProvider>
  );
}