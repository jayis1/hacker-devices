/*
 * lora-phantom / app / App.js
 * Main React Native app with bottom-tab navigation.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * LoRaPhantom companion app — controls the LoRaWAN & LoRa infiltration device
 * over BLE (nRF52840) or USB CDC (STM32H743). Provides screens for:
 *   - Connection (BLE/USB)
 *   - Dashboard (status overview)
 *   - Sniffer (live decoded LoRaWAN frames)
 *   - Key Search (offline MIC brute-force)
 *   - Replay Manager (ABP frame replay)
 *   - Injector (downlink injection + MAC commands)
 *   - Gateway Emulator (rogue network server)
 *   - PHY Fuzzer (LoRa physical-layer fuzzing)
 *   - Spectrum Scanner (channel activity map)
 *   - Settings (region, power, keys, SD, OTA)
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { DeviceProvider } from './components/DeviceContext';
import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import SnifferScreen from './screens/SnifferScreen';
import KeySearchScreen from './screens/KeySearchScreen';
import ReplayScreen from './screens/ReplayScreen';
import InjectorScreen from './screens/InjectorScreen';
import GatewayEmuScreen from './screens/GatewayEmuScreen';
import FuzzerScreen from './screens/FuzzerScreen';
import SpectrumScanScreen from './screens/SpectrumScanScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

const tabOptions = {
  headerStyle: { backgroundColor: '#0a0a0a' },
  headerTintColor: '#00ff88',
  tabBarStyle: { backgroundColor: '#0a0a0a', borderTopColor: '#333' },
  tabBarActiveTintColor: '#00ff88',
  tabBarInactiveTintColor: '#555',
  tabBarLabelStyle: { fontSize: 10 },
};

export default function App() {
  return (
    <SafeAreaProvider>
      <DeviceProvider>
        <NavigationContainer>
          <Tab.Navigator screenOptions={tabOptions}>
            <Tab.Screen name="Connect" component={ConnectionScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="Dashboard" component={DashboardScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="Sniffer" component={SnifferScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="KeySearch" component={KeySearchScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="Replay" component={ReplayScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="Injector" component={InjectorScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="GatewayEmu" component={GatewayEmuScreen}
              options={{ title: 'Gateway', tabBarIcon: () => null }} />
            <Tab.Screen name="Fuzzer" component={FuzzerScreen}
              options={{ tabBarIcon: () => null }} />
            <Tab.Screen name="SpectrumScan" component={SpectrumScanScreen}
              options={{ title: 'Scan', tabBarIcon: () => null }} />
            <Tab.Screen name="Settings" component={SettingsScreen}
              options={{ tabBarIcon: () => null }} />
          </Tab.Navigator>
        </NavigationContainer>
      </DeviceProvider>
    </SafeAreaProvider>
  );
}