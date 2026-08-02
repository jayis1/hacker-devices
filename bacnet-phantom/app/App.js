/**
 * App.js — BACnet Phantom companion app entry point.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * Phone-first operator console for the BACnet Phantom hardware implant.
 * Four screens (Connect / Recon / Inject / Settings) driven by a JSON-over-
 * BLE line protocol. The app is deliberately framework-light so the BLE wire
 * protocol is auditable and reviewable by security researchers.
 *
 * Legal: authorized security research only. Unauthorized use against
 * building-automation systems you do not own or have written permission to
 * test is illegal and may cause physical harm. See README disclaimer.
 */
import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { Ionicons } from '@expo/vector-icons';

import ConnectScreen from './screens/ConnectScreen';
import ReconScreen from './screens/ReconScreen';
import InjectScreen from './screens/InjectScreen';
import SettingsScreen from './screens/SettingsScreen';
import { PhantomProvider } from './src/PhantomContext';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <SafeAreaProvider>
      <PhantomProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={({ route }) => ({
              tabBarIcon: ({ color, size }) => {
                const icons = {
                  Connect: 'bluetooth',
                  Recon: 'radar',
                  Inject: 'create',
                  Settings: 'settings',
                };
                return <Ionicons name={icons[route.name] || 'circle'} size={size} color={color} />;
              },
              tabBarActiveTintColor: '#e63946',
              tabBarInactiveTintColor: 'gray',
              headerTitle: () => null,
            })}
          >
            <Tab.Screen name="Connect" component={ConnectScreen} />
            <Tab.Screen name="Recon" component={ReconScreen} />
            <Tab.Screen name="Inject" component={InjectScreen} />
            <Tab.Screen name="Settings" component={SettingsScreen} />
          </Tab.Navigator>
        </NavigationContainer>
      </PhantomProvider>
    </SafeAreaProvider>
  );
}