/**
 * AppNavigator.js — Stack navigator for RadarPhantom companion app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Defines the navigation stack: Connection -> Dashboard, with sub-screens
 * for scenario editing, target cluster, band sniff, log, and settings.
 */

import React from 'react';
import { createStackNavigator } from '@react-navigation/stack';

import ConnectionScreen from '../screens/ConnectionScreen';
import DashboardScreen from '../screens/DashboardScreen';
import ScenarioEditorScreen from '../screens/ScenarioEditorScreen';
import TargetScreen from '../screens/TargetScreen';
import BandSniffScreen from '../screens/BandSniffScreen';
import LogScreen from '../screens/LogScreen';
import SettingsScreen from '../screens/SettingsScreen';

const Stack = createStackNavigator();

export default function AppNavigator() {
  return (
    <Stack.Navigator
      initialRouteName="Connection"
      screenOptions={{
        headerStyle: { backgroundColor: '#1a1a2e' },
        headerTintColor: '#00ff9f',
        headerTitleStyle: { fontWeight: 'bold' },
        cardStyle: { backgroundColor: '#0f0f1a' },
      }}
    >
      <Stack.Screen
        name="Connection"
        component={ConnectionScreen}
        options={{ title: 'RadarPhantom' }}
      />
      <Stack.Screen
        name="Dashboard"
        component={DashboardScreen}
        options={{ title: 'Phantom Control' }}
      />
      <Stack.Screen
        name="ScenarioEditor"
        component={ScenarioEditorScreen}
        options={{ title: 'Scenario Editor' }}
      />
      <Stack.Screen
        name="Target"
        component={TargetScreen}
        options={{ title: 'Multi-Target Cluster' }}
      />
      <Stack.Screen
        name="BandSniff"
        component={BandSniffScreen}
        options={{ title: 'Band Sniff' }}
      />
      <Stack.Screen
        name="Log"
        component={LogScreen}
        options={{ title: 'Capture Log' }}
      />
      <Stack.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings' }}
      />
    </Stack.Navigator>
  );
}