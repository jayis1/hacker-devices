// Powerline-Reaper companion app — React Native (Expo)
// Author: jayis1
// License: MIT
//
// Entry + navigation. Pairs with the Powerline-Reaper device over BLE
// (covert mode) or via the USB CDC-ECM virtual Ethernet bridge (inline mode).

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StatusBar } from 'react-native';

import DashboardScreen from './screens/DashboardScreen';
import TopologyScreen from './screens/TopologyScreen';
import CaptureScreen from './screens/CaptureScreen';
import InjectScreen from './screens/InjectScreen';
import KeysScreen from './screens/KeysScreen';
import TunnelScreen from './screens/TunnelScreen';
import SettingsScreen from './screens/SettingsScreen';
import { ReaperProvider } from './utils/protocol';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <ReaperProvider>
      <SafeAreaView style={{ flex: 1, backgroundColor: '#0a0a0a' }}>
        <StatusBar barStyle="light-content" backgroundColor="#0a0a0a" />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: { backgroundColor: '#111', borderTopColor: '#222' },
              tabBarActiveTintColor: '#00ffaa',
              tabBarInactiveTintColor: '#666',
              headerStyle: { backgroundColor: '#111' },
              headerTintColor: '#eee',
            }}>
            <Tab.Screen name="Dashboard" component={DashboardScreen}
              options={{ tabBarLabel: 'Status' }} />
            <Tab.Screen name="Topology" component={TopologyScreen} />
            <Tab.Screen name="Capture" component={CaptureScreen} />
            <Tab.Screen name="Inject" component={InjectScreen} />
            <Tab.Screen name="Keys" component={KeysScreen} />
            <Tab.Screen name="Tunnel" component={TunnelScreen} />
            <Tab.Screen name="Settings" component={SettingsScreen} />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaView>
    </ReaperProvider>
  );
}