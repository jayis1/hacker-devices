/**
 * App.js — NVMe-Phantom companion app entry point
 *
 * Author:  jayis1
 * License: MIT
 *
 * Sets up React Native Navigation with all screens and passes a shared
 * BLEManager instance as a prop to every screen.
 */

import React from 'react';
import { Navigation } from 'react-native-navigation';
import BLEManager from './utils/ble';

import ConnectionScreen from './screens/ConnectionScreen';
import DashboardScreen from './screens/DashboardScreen';
import CaptureScreen from './screens/CaptureScreen';
import RulesScreen from './screens/RulesScreen';
import OpalScreen from './screens/OpalScreen';
import SpoofScreen from './screens/SpoofScreen';
import DmaScreen from './screens/DmaScreen';
import CaptureViewerScreen from './screens/CaptureViewerScreen';
import SettingsScreen from './screens/SettingsScreen';

const ble = new BLEManager();

const screens = {
  Connection: ConnectionScreen,
  Dashboard: DashboardScreen,
  Capture: CaptureScreen,
  Rules: RulesScreen,
  Opal: OpalScreen,
  Spoof: SpoofScreen,
  Dma: DmaScreen,
  CaptureViewer: CaptureViewerScreen,
  Settings: SettingsScreen,
};

Navigation.registerComponent('Connection', () => (props) =>
  <ConnectionScreen {...props} ble={ble} />, () => ConnectionScreen);
Navigation.registerComponent('Dashboard', () => (props) =>
  <DashboardScreen {...props} ble={ble} />, () => DashboardScreen);
Navigation.registerComponent('Capture', () => (props) =>
  <CaptureScreen {...props} ble={ble} />, () => CaptureScreen);
Navigation.registerComponent('Rules', () => (props) =>
  <RulesScreen {...props} ble={ble} />, () => RulesScreen);
Navigation.registerComponent('Opal', () => (props) =>
  <OpalScreen {...props} ble={ble} />, () => OpalScreen);
Navigation.registerComponent('Spoof', () => (props) =>
  <SpoofScreen {...props} ble={ble} />, () => SpoofScreen);
Navigation.registerComponent('Dma', () => (props) =>
  <DmaScreen {...props} ble={ble} />, () => DmaScreen);
Navigation.registerComponent('CaptureViewer', () => (props) =>
  <CaptureViewerScreen {...props} ble={ble} />, () => CaptureViewerScreen);
Navigation.registerComponent('Settings', () => (props) =>
  <SettingsScreen {...props} ble={ble} />, () => SettingsScreen);

Navigation.events().registerAppLaunchedListener(() => {
  Navigation.setRoot({
    root: {
      stack: {
        children: [{
          component: { name: 'Connection' }
        }],
        options: {
          topBar: { title: { text: 'NVMe-Phantom' } }
        }
      }
    }
  });
});