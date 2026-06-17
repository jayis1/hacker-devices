/**
 * App.js — FiberPhantom Companion App
 * React Native app for controlling the FiberPhantom covert fiber optic tap
 * via BLE 5.0 connection to the nRF52840 module.
 *
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Features:
 *  - BLE device scanning and connection
 *  - Dashboard: live status, battery, link rate, capture stats
 *  - Tap Configuration: APD bias, AGC, capture mode, link type
 *  - MITM Rules: create/edit/delete frame modification rules
 *  - Capture: start/stop capture, live frame count, PCAP download
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { BleManager } from 'react-native-ble-plx';
import { Text, View, StyleSheet, Alert } from 'react-native';

// Screens
import DashboardScreen from './screens/DashboardScreen';
import TapConfigScreen from './screens/TapConfigScreen';
import MitmRulesScreen from './screens/MitmRulesScreen';
import CaptureScreen from './screens/CaptureScreen';

// BLE Protocol utilities
import { FiberPhantomBLE, C2_MSG_TYPE } from './utils/protocol';

const Tab = createBottomTabNavigator();

export default function App() {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState({
    mode: 'standby',
    state: 'idle',
    batteryPct: 0,
    charging: false,
    sdInserted: false,
    sdFreeMB: 0,
    bleConnected: false,
    fpgaReady: false,
    linkRate: 'down',
    stats: {
      totalFrames: 0,
      droppedFrames: 0,
      mitmModified: 0,
      mitmDropped: 0,
      bytesCaptured: 0,
      crcErrors: 0,
      uptimeSec: 0,
    },
  });
  const [alerts, setAlerts] = useState([]);
  const [scanning, setScanning] = useState(false);

  // BLE manager instance
  const bleManager = useRef(new BleManager()).current;
  const ble = useRef(new FiberPhantomBLE(bleManager)).current;

  // Scan for FiberPhantom devices
  const scanForDevices = useCallback(() => {
    setScanning(true);
    bleManager.startDeviceScan(null, { allowDuplicates: false }, (error, scannedDevice) => {
      if (error) {
        console.error('Scan error:', error);
        setScanning(false);
        return;
      }
      if (scannedDevice && scannedDevice.name === 'FiberPhantom') {
        bleManager.stopDeviceScan();
        setScanning(false);
        connectToDevice(scannedDevice);
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      bleManager.stopDeviceScan();
      setScanning(false);
    }, 10000);
  }, [bleManager]);

  // Connect to a FiberPhantom device
  const connectToDevice = useCallback(async (scannedDevice) => {
    try {
      const connectedDevice = await scannedDevice.connect({
        requestMTU: 247,
        autoDiscover: true,
      });
      await connectedDevice.discoverAllServicesAndCharacteristics();
      setDevice(connectedDevice);
      setConnected(true);

      // Setup notification listener for RX characteristic
      connectedDevice.monitorCharacteristicForService(
        FiberPhantomBLE.SERVICE_UUID,
        FiberPhantomBLE.RX_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            console.error('Monitor error:', error);
            return;
          }
          if (characteristic && characteristic.value) {
            const messages = ble.processNotification(characteristic.value);
            messages.forEach(msg => handleMessage(msg));
          }
        }
      );

      // Start periodic status polling
      const interval = setInterval(async () => {
        if (connected) {
          await ble.sendCommand(C2_MSG_TYPE.GET_STATUS, []);
          await ble.sendCommand(C2_MSG_TYPE.GET_STATS, []);
        }
      }, 2000);

      return () => clearInterval(interval);
    } catch (error) {
      console.error('Connection error:', error);
      Alert.alert('Connection Failed', 'Could not connect to FiberPhantom device');
    }
  }, [ble, connected]);

  // Handle incoming C2 messages
  const handleMessage = useCallback((msg) => {
    switch (msg.type) {
      case C2_MSG_TYPE.STATUS:
        setStatus(prev => ({
          ...prev,
          mode: parseMode(msg.payload[0]),
          state: parseState(msg.payload[1]),
          batteryPct: msg.payload[2],
          charging: msg.payload[3] === 1,
          sdInserted: msg.payload[4] === 1,
          sdFreeMB: msg.payload[5] | (msg.payload[6] << 8),
          bleConnected: msg.payload[7] === 1,
          fpgaReady: msg.payload[8] === 1,
          linkRate: parseLinkRate(msg.payload[9]),
        }));
        break;

      case C2_MSG_TYPE.STATS:
        const totalFrames = readU32(msg.payload, 0);
        const droppedFrames = readU32(msg.payload, 4);
        const mitmModified = readU32(msg.payload, 8);
        const mitmDropped = readU32(msg.payload, 12);
        const crcErrors = readU32(msg.payload, 16);
        const uptimeSec = readU32(msg.payload, 20);
        setStatus(prev => ({
          ...prev,
          stats: { totalFrames, droppedFrames, mitmModified, mitmDropped, crcErrors, uptimeSec },
        }));
        break;

      case C2_MSG_TYPE.ALERT:
        const alertCode = msg.payload[0];
        const severity = msg.payload[1];
        const alertInfo = parseAlert(alertCode, severity);
        setAlerts(prev => [...prev, { ...alertInfo, id: Date.now() }]);
        if (severity >= 2) {
          Alert.alert('⚠️ ' + alertInfo.title, alertInfo.message);
        }
        break;

      case C2_MSG_TYPE.CALIB_RESULT:
        const biasMV = readU32(msg.payload, 0);
        Alert.alert('Calibration Complete', `Optimal APD bias: ${biasMV} mV`);
        break;

      case C2_MSG_TYPE.VERSION:
        const ver = String.fromCharCode(...msg.payload.slice(0, 14));
        console.log('Firmware version:', ver);
        break;

      default:
        console.log('Received message type:', msg.type, 'len:', msg.length);
    }
  }, []);

  // Auto-scan on mount
  useEffect(() => {
    scanForDevices();
    return () => {
      if (device) device.cancelConnection();
      bleManager.destroy();
    };
  }, []);

  // Clear old alerts after 30 seconds
  useEffect(() => {
    if (alerts.length > 0) {
      const timer = setTimeout(() => {
        setAlerts(prev => prev.filter(a => Date.now() - a.id < 30000));
      }, 30000);
      return () => clearTimeout(timer);
    }
  }, [alerts]);

  // Pass down to screens
  const screenProps = {
    ble,
    device,
    connected,
    scanning,
    status,
    alerts,
    onScan: scanForDevices,
    onConnect: connectToDevice,
  };

  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={({ route }) => ({
          tabBarIcon: ({ focused, color, size }) => {
            let iconName;
            switch (route.name) {
              case 'Dashboard': iconName = focused ? 'view-dashboard' : 'view-dashboard-outline'; break;
              case 'Tap Config': iconName = focused ? 'tune' : 'tune-vertical'; break;
              case 'MITM Rules': iconName = focused ? 'filter-variant' : 'filter-variant-plus'; break;
              case 'Capture': iconName = focused ? 'record-rec' : 'record'; break;
              default: iconName = 'circle';
            }
            return <Icon name={iconName} size={size} color={color} />;
          },
          tabBarActiveTintColor: '#00ff88',
          tabBarInactiveTintColor: '#666',
          tabBarStyle: { backgroundColor: '#0d1117', borderTopColor: '#30363d' },
          headerStyle: { backgroundColor: '#0d1117' },
          headerTintColor: '#e6edf3',
        })}
      >
        <Tab.Screen name="Dashboard">
          {() => <DashboardScreen {...screenProps} />}
        </Tab.Screen>
        <Tab.Screen name="Tap Config">
          {() => <TapConfigScreen {...screenProps} />}
        </Tab.Screen>
        <Tab.Screen name="MITM Rules">
          {() => <MitmRulesScreen {...screenProps} />}
        </Tab.Screen>
        <Tab.Screen name="Capture">
          {() => <CaptureScreen {...screenProps} />}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}

// Helper functions
function parseMode(m) {
  const modes = ['Bend-Tap', 'Inline-Couple', 'SFP+ Pass-Through', 'Standby'];
  return modes[m] || 'Unknown';
}

function parseState(s) {
  const states = ['Boot', 'Config FPGA', 'Idle', 'Capturing', 'MITM Active', 'SD Full', 'Low Battery', 'Error'];
  return states[s] || 'Unknown';
}

function parseLinkRate(r) {
  const rates = ['Down', '1G', '10G', 'FC 8G', 'FC 16G'];
  return rates[r] || 'Unknown';
}

function parseAlert(code, severity) {
  const alerts = {
    1: { title: 'SD Card Full', message: 'Storage is full. Capture stopped.' },
    2: { title: 'SD Card Removed', message: 'SD card was removed during operation.' },
    3: { title: 'Low Battery', message: 'Battery is low. Consider recharging.' },
    4: { title: 'Battery Critical', message: 'Battery critically low. Device entering standby.' },
    5: { title: 'APD Fault', message: 'Avalanche photodiode fault detected.' },
    6: { title: 'FPGA Error', message: 'FPGA configuration error.' },
    7: { title: 'Link Lost', message: 'Optical link was lost.' },
    8: { title: 'MITM Match', message: 'A MITM rule was triggered.' },
    9: { title: 'FIFO Overflow', message: 'Frame FIFO overflow — frames dropped.' },
    10: { title: 'Overheat', message: 'Device temperature exceeds safe limit.' },
  };
  return alerts[code] || { title: 'Unknown Alert', message: `Alert code: ${code}` };
}

function readU32(buf, offset) {
  return ((buf[offset] << 24) | (buf[offset + 1] << 16) | (buf[offset + 2] << 8) | buf[offset + 3]) >>> 0;
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1117',
  },
});

// Import useRef at top level — fixing the import
import { useRef } from 'react';