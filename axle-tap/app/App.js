// App.js — AxleTap companion app entry point
// Author: jayis1
// License: MIT
// Date:   2026-07-22
//
// Bottom-tab navigator over the five main screens.

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { Text } from 'react-native';

import DashboardScreen from './screens/DashboardScreen';
import CaptureScreen from './screens/CaptureScreen';
import GptpScreen from './screens/GptpScreen';
import TsnScreen from './screens/TsnScreen';
import SomeipScreen from './screens/SomeipScreen';
import ConsoleScreen from './screens/ConsoleScreen';

const Tab = createBottomTabNavigator();

function TabLabel({ focused, label, color }) {
  return (
    <Text style={{ color, fontSize: 10, fontWeight: focused ? 'bold' : 'normal', fontFamily: 'monospace' }}>
      {label}
    </Text>
  );
}

export default function App() {
  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarStyle: { backgroundColor: '#1a1a1a', borderTopColor: '#333' },
            headerStyle: { backgroundColor: '#1a1a1a' },
            headerTintColor: '#7ab8ff',
            tabBarActiveTintColor: '#7ab8ff',
            tabBarInactiveTintColor: '#888',
          }}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ tabBarLabel: ({ color, focused }) => <TabLabel color={color} focused={focused} label="Dash" /> }}
          />
          <Tab.Screen
            name="Capture"
            component={CaptureScreen}
            options={{ tabBarLabel: ({ color, focused }) => <TabLabel color={color} focused={focused} label="Cap" /> }}
          />
          <Tab.Screen
            name="gPTP"
            component={GptpScreen}
            options={{ tabBarLabel: ({ color, focused }) => <TabLabel color={color} focused={focused} label="gPTP" /> }}
          />
          <Tab.Screen
            name="TSN"
            component={TsnScreen}
            options={{ tabBarLabel: ({ color, focused }) => <TabLabel color={color} focused={focused} label="TSN" /> }}
          />
          <Tab.Screen
            name="SOME/IP"
            component={SomeipScreen}
            options={{ tabBarLabel: ({ color, focused }) => <TabLabel color={color} focused={focused} label="SOME" /> }}
          />
          <Tab.Screen
            name="Console"
            component={ConsoleScreen}
            options={{ tabBarLabel: ({ color, focused }) => <TabLabel color={color} focused={focused} label="CLI" /> }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}