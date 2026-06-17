/**
 * App.tsx — Forge-Probe Companion App Main Entry Point
 * Author: jayis1
 * License: MIT
 *
 * Root navigation container for the Forge-Probe mobile app.
 * Uses React Navigation's stack navigator for screen management.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { StatusBar } from 'react-native';

import DashboardScreen from './screens/DashboardScreen';
import MemoryViewScreen from './screens/MemoryViewScreen';

/* ─── Navigation Type Definitions ──────────────────────────────────────────── */

export type RootStackParamList = {
  Dashboard: undefined;
  MemoryView: undefined;
  FlashDump: undefined;
  RegisterView: undefined;
  BoundaryScan: undefined;
  Settings: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

/* ─── App Component ────────────────────────────────────────────────────────── */

const App: React.FC = () => {
  return (
    <NavigationContainer>
      <StatusBar barStyle="light-content" backgroundColor="#121212" />
      <Stack.Navigator
        initialRouteName="Dashboard"
        screenOptions={{
          headerStyle: {
            backgroundColor: '#1A1A1A',
            shadowColor: 'transparent',
            elevation: 0,
          },
          headerTintColor: '#00BCD4',
          headerTitleStyle: {
            fontWeight: '700',
            fontSize: 16,
          },
          cardStyle: {
            backgroundColor: '#121212',
          },
        }}
      >
        <Stack.Screen
          name="Dashboard"
          component={DashboardScreen}
          options={{
            headerShown: false,
          }}
        />
        <Stack.Screen
          name="MemoryView"
          component={MemoryViewScreen}
          options={{
            title: 'Memory Explorer',
          }}
        />
        <Stack.Screen
          name="FlashDump"
          component={MemoryViewScreen}
          options={{
            title: 'Flash Dump',
          }}
        />
        <Stack.Screen
          name="RegisterView"
          component={MemoryViewScreen}
          options={{
            title: 'Core Registers',
          }}
        />
        <Stack.Screen
          name="BoundaryScan"
          component={MemoryViewScreen}
          options={{
            title: 'Boundary Scan',
          }}
        />
        <Stack.Screen
          name="Settings"
          component={MemoryViewScreen}
          options={{
            title: 'Settings',
          }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
};

export default App;