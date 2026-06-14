/**
 * App.js — CAN Bus Infiltrator companion app
 * React Native with bottom tab navigation
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import SnifferScreen from './screens/SnifferScreen';
import InjectScreen from './screens/InjectScreen';
import FuzzerScreen from './screens/FuzzerScreen';
import DeviceScreen from './screens/DeviceScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={{
          tabBarActiveTintColor: '#00E676',
          tabBarInactiveTintColor: '#757575',
          tabBarStyle: { backgroundColor: '#1A1A2E' },
          headerStyle: { backgroundColor: '#16213E' },
          headerTintColor: '#FFFFFF',
        }}
      >
        <Tab.Screen
          name="Sniffer"
          component={SnifferScreen}
          options={{
            tabBarLabel: 'Sniffer',
            tabBarIcon: ({ color, size }) => (
              <Icon name="eye-outline" color={color} size={size} />
            ),
          }}
        />
        <Tab.Screen
          name="Inject"
          component={InjectScreen}
          options={{
            tabBarLabel: 'Inject',
            tabBarIcon: ({ color, size }) => (
              <Icon name="send" color={color} size={size} />
            ),
          }}
        />
        <Tab.Screen
          name="Fuzzer"
          component={FuzzerScreen}
          options={{
            tabBarLabel: 'Fuzzer',
            tabBarIcon: ({ color, size }) => (
              <Icon name="shuffle-variant" color={color} size={size} />
            ),
          }}
        />
        <Tab.Screen
          name="Device"
          component={DeviceScreen}
          options={{
            tabBarLabel: 'Device',
            tabBarIcon: ({ color, size }) => (
              <Icon name="cog-outline" color={color} size={size} />
            ),
          }}
        />
      </Tab.Navigator>
    </NavigationContainer>
  );
}