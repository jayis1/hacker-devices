// App.tsx — Root entry for the CC-Stiletto companion app.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
//
// CC-Stiletto is a USB-C Power Delivery Configuration Channel attack tool for
// authorized security research. This app provides a mobile control surface for
// the device over USB-CDC.

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { Provider as PaperProvider } from 'react-native-paper';

import SnifferScreen    from './src/screens/SnifferScreen';
import CapabilityScreen from './src/screens/CapabilityScreen';
import GlitchScreen     from './src/screens/GlitchScreen';
import HijackScreen     from './src/screens/HijackScreen';
import TelemetryScreen  from './src/screens/TelemetryScreen';

import { DeviceProvider } from './src/state/DeviceContext';
import { LegalBanner } from './src/components/LegalBanner';

export type RootTabParamList = {
  Sniffer: undefined;
  Capabilities: undefined;
  Glitch: undefined;
  Hijack: undefined;
  Telemetry: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

export default function App() {
  return (
    <SafeAreaProvider>
      <PaperProvider>
        <DeviceProvider>
          <LegalBanner />
          <NavigationContainer>
            <Tab.Navigator
              initialRouteName="Sniffer"
              screenOptions={{ headerShown: true, tabBarActiveTintColor: '#d62828' }}
            >
              <Tab.Screen
                name="Sniffer"
                component={SnifferScreen}
                options={{ title: 'PD Sniffer' }}
              />
              <Tab.Screen
                name="Capabilities"
                component={CapabilityScreen}
                options={{ title: 'Spoof / Inject' }}
              />
              <Tab.Screen
                name="Glitch"
                component={GlitchScreen}
                options={{ title: 'Glitch' }}
              />
              <Tab.Screen
                name="Hijack"
                component={HijackScreen}
                options={{ title: 'Role Hijack' }}
              />
              <Tab.Screen
                name="Telemetry"
                component={TelemetryScreen}
                options={{ title: 'Telemetry' }}
              />
            </Tab.Navigator>
          </NavigationContainer>
        </DeviceProvider>
      </PaperProvider>
    </SafeAreaProvider>
  );
}