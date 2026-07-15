/**
 * TeslaPhantom — React Native Companion App
 * Wireless control for the EMFI & Magnetic SCA Platform.
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * App entry point — sets up navigation and BLE device context.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { Provider as DeviceProvider } from './components/DeviceContext';

import DashboardScreen from './screens/DashboardScreen';
import EmfiControlScreen from './screens/EmfiControlScreen';
import ScaCaptureScreen from './screens/ScaCaptureScreen';
import ScanSetupScreen from './screens/ScanSetupScreen';
import ScanMapScreen from './screens/ScanMapScreen';
import TraceAnalysisScreen from './screens/TraceAnalysisScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Tab.Navigator
          initialRouteName="Dashboard"
          screenOptions={{
            tabBarActiveTintColor: '#e74c3c',
            tabBarInactiveTintColor: '#95a5a6',
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#fff',
          }}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'TeslaPhantom' }}
          />
          <Tab.Screen
            name="EMFI"
            component={EmfiControlScreen}
            options={{ title: 'EMFI Control' }}
          />
          <Tab.Screen
            name="SCA"
            component={ScaCaptureScreen}
            options={{ title: 'SCA Capture' }}
          />
          <Tab.Screen
            name="ScanSetup"
            component={ScanSetupScreen}
            options={{ title: 'Scan Setup' }}
          />
          <Tab.Screen
            name="ScanMap"
            component={ScanMapScreen}
            options={{ title: 'Fault Map' }}
          />
          <Tab.Screen
            name="Trace"
            component={TraceAnalysisScreen}
            options={{ title: 'Trace Analysis' }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}