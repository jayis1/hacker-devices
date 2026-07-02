// App.tsx — Aurora-Phantom companion app root
// Author: jayis1   License: GPL-2.0
//
// Bottom-tab navigator over the seven operator screens.

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';

import ConnectScreen from './screens/ConnectScreen';
import MissionScreen from './screens/MissionScreen';
import LiveSpectrumScreen from './screens/LiveSpectrumScreen';
import ReconstructionScreen from './screens/ReconstructionScreen';
import EventLogScreen from './screens/EventLogScreen';
import StorageScreen from './screens/StorageScreen';
import SettingsScreen from './screens/SettingsScreen';

export type RootTabParamList = {
  Connect: undefined;
  Mission: undefined;
  Spectrum: undefined;
  Reconstruction: undefined;
  EventLog: undefined;
  Storage: undefined;
  Settings: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

export default function App() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={{
          headerStyle: { backgroundColor: '#0a0a12' },
          headerTintColor: '#7fd4ff',
          tabBarStyle: { backgroundColor: '#0a0a12' },
          tabBarActiveTintColor: '#7fd4ff',
          tabBarInactiveTintColor: '#555',
        }}
      >
        <Tab.Screen name="Connect" component={ConnectScreen}
          options={{ title: 'Connect' }} />
        <Tab.Screen name="Mission" component={MissionScreen}
          options={{ title: 'Mission' }} />
        <Tab.Screen name="Spectrum" component={LiveSpectrumScreen}
          options={{ title: 'Spectrum' }} />
        <Tab.Screen name="Reconstruction" component={ReconstructionScreen}
          options={{ title: 'Reconstruct' }} />
        <Tab.Screen name="EventLog" component={EventLogScreen}
          options={{ title: 'Events' }} />
        <Tab.Screen name="Storage" component={StorageScreen}
          options={{ title: 'Storage' }} />
        <Tab.Screen name="Settings" component={SettingsScreen}
          options={{ title: 'Settings' }} />
      </Tab.Navigator>
    </NavigationContainer>
  );
}