/**
 * AperturePhantom — companion app (React Native)
 *
 * Root component wiring the DeviceContext provider and the stack
 * navigator over all screens. BLE/USB transport lives in utils/protocol.js
 * and components/DeviceContext.js.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { DeviceProvider } from './components/DeviceContext';

import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import LiveViewScreen from './screens/LiveViewScreen';
import ReplayLibraryScreen from './screens/ReplayLibraryScreen';
import InjectEditorScreen from './screens/InjectEditorScreen';
import ScriptEditorScreen from './screens/ScriptEditorScreen';
import SensorConsoleScreen from './screens/SensorConsoleScreen';
import FuzzerScreen from './screens/FuzzerScreen';

const Stack = createStackNavigator();

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Connection"
          screenOptions={{
            headerStyle: { backgroundColor: '#101418' },
            headerTintColor: '#d0e0f0',
            headerTitleStyle: { fontWeight: '600' },
          }}>
          <Stack.Screen name="Connection"  component={ConnectionScreen}
            options={{ title: 'AperturePhantom · Connect' }} />
          <Stack.Screen name="Dashboard"   component={DashboardScreen}
            options={{ title: 'Dashboard' }} />
          <Stack.Screen name="LiveView"    component={LiveViewScreen}
            options={{ title: 'Live View' }} />
          <Stack.Screen name="Replay"       component={ReplayLibraryScreen}
            options={{ title: 'Replay Library' }} />
          <Stack.Screen name="Inject"       component={InjectEditorScreen}
            options={{ title: 'Inject Editor' }} />
          <Stack.Screen name="Script"      component={ScriptEditorScreen}
            options={{ title: 'Script Editor' }} />
          <Stack.Screen name="Sensor"       component={SensorConsoleScreen}
            options={{ title: 'Sensor Console' }} />
          <Stack.Screen name="Fuzzer"       component={FuzzerScreen}
            options={{ title: 'CSI-2 Fuzzer' }} />
        </Stack.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}