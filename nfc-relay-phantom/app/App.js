/**
 * NFC Relay Phantom — React Native Companion App
 * Controls the NFC Relay Phantom hardware device via BLE
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// Screens
import HomeScreen from './screens/HomeScreen';
import NFCScreen from './screens/NFCScreen';
import RFID125Screen from './screens/RFID125Screen';
import RelayScreen from './screens/RelayScreen';
import SettingsScreen from './screens/SettingsScreen';

// BLE Protocol
import { PhantomBLEManager } from './utils/protocol';

const Tab = createBottomTabNavigator();

// Global BLE manager instance
export const bleManager = new PhantomBLEManager();

export default function App() {
  return (
    <PaperProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ color, size }) => {
              let iconName;
              switch (route.name) {
                case 'Home':
                  iconName = 'home';
                  break;
                case 'NFC':
                  iconName = 'nfc-variant';
                  break;
                case 'RFID125':
                  iconName = 'rfid';
                  break;
                case 'Relay':
                  iconName = 'swap-horizontal';
                  break;
                case 'Settings':
                  iconName = 'cog';
                  break;
                default:
                  iconName = 'chip';
              }
              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#6200EE',
            tabBarInactiveTintColor: 'gray',
            headerStyle: { backgroundColor: '#6200EE' },
            headerTintColor: '#fff',
          })}
        >
          <Tab.Screen name="Home" component={HomeScreen} />
          <Tab.Screen name="NFC" component={NFCScreen} />
          <Tab.Screen name="RFID125" component={RFID125Screen} />
          <Tab.Screen name="Relay" component={RelayScreen} />
          <Tab.Screen name="Settings" component={SettingsScreen} />
        </Tab.Navigator>
      </NavigationContainer>
    </PaperProvider>
  );
}