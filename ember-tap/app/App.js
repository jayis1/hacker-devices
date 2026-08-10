/**
 * App.js — Ember-Tap companion app entry point
 *
 * Author: jayis1
 * License: MIT
 *
 * Bottom-tab navigation across 6 screens:
 *   Dashboard | Sniffer | Fuzzer | Spoof | Glitch | Logs
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialIcons';

import DashboardScreen from './screens/DashboardScreen';
import SnifferScreen from './screens/SnifferScreen';
import FuzzerScreen from './screens/FuzzerScreen';
import SpoofScreen from './screens/SpoofScreen';
import GlitchScreen from './screens/GlitchScreen';
import LogsScreen from './screens/LogsScreen';

import { EmberContext } from './utils/protocol';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={({ route }) => ({
          tabBarIcon: ({ focused, color, size }) => {
            let iconName;
            switch (route.name) {
              case 'Dashboard': iconName = 'dashboard'; break;
              case 'Sniffer':   iconName = 'visibility'; break;
              case 'Fuzzer':    iconName = 'bug-report'; break;
              case 'Spoof':     iconName = 'flash-on'; break;
              case 'Glitch':    iconName = 'bolt'; break;
              case 'Logs':      iconName = 'list'; break;
              default:          iconName = 'help';
            }
            return <Icon name={iconName} size={size} color={color} />;
          },
          tabBarActiveTintColor: '#FF6600',
          tabBarInactiveTintColor: 'gray',
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#fff',
        })}
      >
        <Tab.Screen name="Dashboard" component={DashboardScreen}
          options={{ title: 'Ember-Tap' }} />
        <Tab.Screen name="Sniffer" component={SnifferScreen} />
        <Tab.Screen name="Fuzzer" component={FuzzerScreen} />
        <Tab.Screen name="Spoof" component={SpoofScreen} />
        <Tab.Screen name="Glitch" component={GlitchScreen} />
        <Tab.Screen name="Logs" component={LogsScreen} />
      </Tab.Navigator>
    </NavigationContainer>
  );
}

/* end of file — author: jayis1 */