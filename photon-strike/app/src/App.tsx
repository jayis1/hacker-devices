/**
 * App.tsx — PhotonStrike companion app entry point
 * Author: jayis1
 * License: MIT
 *
 * Sets up the navigation stack and the BLE connection context.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { GestureHandlerRootView } from 'react-native-gesture-handler';

import { BleProvider } from './src/services/BleContext';
import HomeScreen from './src/screens/HomeScreen';
import ScanSetupScreen from './src/screens/ScanSetupScreen';
import TriggerScreen from './src/screens/TriggerScreen';
import ScanProgressScreen from './src/screens/ScanProgressScreen';
import DFAScreen from './src/screens/DFAScreen';
import LogScreen from './src/screens/LogScreen';
import SafetyScreen from './src/screens/SafetyScreen';

export type RootStackParamList = {
  Home: undefined;
  ScanSetup: undefined;
  Trigger: undefined;
  ScanProgress: undefined;
  DFA: undefined;
  Log: undefined;
  Safety: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

export default function App() {
  return (
    <GestureHandlerRootView style={{ flex: 1 }}>
      <BleProvider>
        <NavigationContainer>
          <Stack.Navigator
            initialRouteName="Home"
            screenOptions={{
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#e94560',
              headerTitleStyle: { fontWeight: 'bold' },
              cardStyle: { backgroundColor: '#0f0f1e' },
            }}
          >
            <Stack.Screen name="Home" component={HomeScreen} options={{ title: 'PhotonStrike' }} />
            <Stack.Screen name="ScanSetup" component={ScanSetupScreen} options={{ title: 'Scan Setup' }} />
            <Stack.Screen name="Trigger" component={TriggerScreen} options={{ title: 'Trigger Config' }} />
            <Stack.Screen name="ScanProgress" component={ScanProgressScreen} options={{ title: 'Scan Progress' }} />
            <Stack.Screen name="DFA" component={DFAScreen} options={{ title: 'DFA Key Recovery' }} />
            <Stack.Screen name="Log" component={LogScreen} options={{ title: 'Scan Logs' }} />
            <Stack.Screen name="Safety" component={SafetyScreen} options={{ title: 'Laser Safety' }} />
          </Stack.Navigator>
        </NavigationContainer>
      </BleProvider>
    </GestureHandlerRootView>
  );
}