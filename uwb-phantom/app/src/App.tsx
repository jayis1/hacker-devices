/**
 * App.tsx — UWB-PHANTOM companion app entry point.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Tab-based navigation between the seven main screens:
 *  Connect, Dashboard, TWR, Sniffer, STSAudit, Relay, TrackerScan,
 *  plus a Settings stack and a Targets manager.
 *
 * The app pairs to UWB-PHANTOM hardware over BLE using the custom
 * 0xFEA5 service.  All commands are sent as tiny JSON objects on the
 * `cmd` characteristic; events arrive on `event` (notifications).
 *
 * Legal acknowledgement: on first launch the user must accept an
 * "authorized use only" dialog before any BLE scanning begins.
 */

import React, { useState } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider, Text } from 'react-native-paper';

import { BleContext, BleDevice } from './src/ble/UwbPhantomBle';
import ConnectScreen from './src/screens/ConnectScreen';
import DashboardScreen from './src/screens/DashboardScreen';
import TwrScreen from './src/screens/TwrScreen';
import SnifferScreen from './src/screens/SnifferScreen';
import StsAuditScreen from './src/screens/StsAuditScreen';
import RelayScreen from './src/screens/RelayScreen';
import TrackerScanScreen from './src/screens/TrackerScanScreen';
import TargetsScreen from './src/screens/TargetsScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import LegalGate from './src/components/LegalGate';

const Tab = createBottomTabNavigator();

export default function App(): JSX.Element {
  const [device, setDevice] = useState<BleDevice | null>(null);

  return (
    <PaperProvider>
      <LegalGate>
        <BleContext.Provider value={{ device, setDevice }}>
          <NavigationContainer>
            <Tab.Navigator initialRouteName="Connect">
              <Tab.Screen name="Connect"  component={ConnectScreen}
                options={{ title: 'Connect' }} />
              <Tab.Screen name="Dashboard" component={DashboardScreen}
                options={{ title: 'Dashboard' }} />
              <Tab.Screen name="TWR" component={TwrScreen}
                options={{ title: 'TWR' }} />
              <Tab.Screen name="Sniffer" component={SnifferScreen}
                options={{ title: 'Sniffer' }} />
              <Tab.Screen name="STS Audit" component={StsAuditScreen}
                options={{ title: 'STS Audit' }} />
              <Tab.Screen name="Relay" component={RelayScreen}
                options={{ title: 'Relay' }} />
              <Tab.Screen name="Tracker" component={TrackerScanScreen}
                options={{ title: 'Tracker' }} />
              <Tab.Screen name="Targets" component={TargetsScreen}
                options={{ title: 'Targets' }} />
              <Tab.Screen name="Settings" component={SettingsScreen}
                options={{ title: 'Settings' }} />
            </Tab.Navigator>
          </NavigationContainer>
        </BleContext.Provider>
      </LegalGate>
    </PaperProvider>
  );
}