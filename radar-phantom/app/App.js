/**
 * App.js — RadarPhantom companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Main navigation and app structure for the RadarPhantom mmWave radar
 * phantom-target injector companion app. Provides BLE connection management
 * and navigation to all scenario/target/sniff/log screens.
 *
 * Legal: for authorized security research only. See README disclaimer.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import { DeviceProvider } from './src/components/DeviceContext';
import AppNavigator from './src/navigation/AppNavigator';

const Stack = createStackNavigator();

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <AppNavigator />
      </NavigationContainer>
    </DeviceProvider>
  );
}