/**
 * ACOUSTIC-PHANTOM — Main App entry point
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Sets up navigation, BLE context provider, and the 5-screen
 * tab navigator for the companion app.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { BLEProvider } from './utils/ble';
import DashboardScreen from './screens/DashboardScreen';
import EventFeedScreen from './screens/EventFeedScreen';
import SpectrogramScreen from './screens/SpectrogramScreen';
import CalibrationScreen from './screens/CalibrationScreen';
import CaptureManagerScreen from './screens/CaptureManagerScreen';

const Tab = createBottomTabNavigator();

const screenOptions = {
  tabBarActiveTintColor: '#00AAFF',
  tabBarInactiveTintColor: '#666',
  tabBarStyle: { backgroundColor: '#111', borderTopColor: '#333' },
  headerStyle: { backgroundColor: '#111' },
  headerTintColor: '#FFF',
  headerTitleStyle: { fontWeight: 'bold' },
};

export default function App() {
  return (
    <SafeAreaProvider>
      <BLEProvider>
        <NavigationContainer>
          <Tab.Navigator screenOptions={screenOptions}>
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{ title: 'ACOUSTIC-PHANTOM' }}
            />
            <Tab.Screen
              name="Events"
              component={EventFeedScreen}
              options={{ title: 'Live Event Feed' }}
            />
            <Tab.Screen
              name="Spectrogram"
              component={SpectrogramScreen}
              options={{ title: 'Spectrogram' }}
            />
            <Tab.Screen
              name="Calibration"
              component={CalibrationScreen}
              options={{ title: 'Calibration' }}
            />
            <Tab.Screen
              name="Capture Manager"
              component={CaptureManagerScreen}
              options={{ title: 'Captures' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </BLEProvider>
    </SafeAreaProvider>
  );
}