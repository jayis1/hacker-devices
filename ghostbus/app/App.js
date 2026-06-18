/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — Root component & navigation
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

import DashboardScreen from './screens/DashboardScreen';
import DiscoveryScreen from './screens/DiscoveryScreen';
import MemoryScreen from './screens/MemoryScreen';
import ConsoleScreen from './screens/ConsoleScreen';
import { GhostBusService } from './utils/protocol';
import { StatusBar } from './components/StatusBar';

/**
 * GHOSTBUS BLE service UUIDs — custom GATT service
 * Matches firmware/drivers/ble_bridge.h
 * Author: jayis1
 */
const GB_SERVICE_UUID = '8a7b1c2d-3e4f-5061-7283-94a5b6c7d8e9';
const GB_CHAR_COMMAND  = '8a7b1c2d-3e4f-5061-7283-94a5b6c7d8ea';
const GB_CHAR_TELEMETRY = '8a7b1c2d-3e4f-5061-7283-94a5b6c7d8eb';
const GB_CHAR_DATA      = '8a7b1c2d-3e4f-5061-7283-94a5b6c7d8ec';
const GB_CHAR_AUTH      = '8a7b1c2d-3e4f-5061-7283-94a5b6c7d8ed';

const Tab = createBottomTabNavigator();

export default function App() {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [authenticated, setAuthenticated] = useState(false);
  const [battery, setBattery] = useState(0);
  const [targetState, setTargetState] = useState('IDLE');
  const [discovery, setDiscovery] = useState(null);
  const [flashDump, setFlashDump] = useState(null);
  const [logs, setLogs] = useState([]);
  const [service, setService] = useState(null);

  const bleManager = React.useRef(new BleManager()).current;

  /**
   * Log helper — append timestamped message to the session log.
   */
  const log = useCallback((msg, level = 'info') => {
    const ts = new Date().toISOString().substr(11, 8);
    setLogs(prev => [...prev, { ts, msg, level }].slice(-200));
  }, []);

  /**
   * Connect to the GHOSTBUS device via BLE.
   * Scans for devices advertising the GHOSTBUS service UUID.
   */
  const connect = useCallback(async () => {
    log('Scanning for GHOSTBUS...');
    bleManager.startDeviceScan([GB_SERVICE_UUID], null, (error, dev) => {
      if (error) {
        log(`Scan error: ${error.message}`, 'error');
        return;
      }
      if (dev && dev.name && dev.name.includes('GHOSTBUS')) {
        log(`Found ${dev.name} (${dev.id})`);
        bleManager.stopDeviceScan();
        dev.connect()
          .then(d => d.discoverAllServicesAndCharacteristics())
          .then(d => {
            setDevice(d);
            setConnected(true);
            log('Connected to GHOSTBUS');
            // Subscribe to telemetry
            d.monitorCharacteristicForService(
              GB_SERVICE_UUID, GB_CHAR_TELEMETRY,
              (err, char) => {
                if (err) return;
                if (char && char.value) {
                  const buf = Buffer.from(char.value, 'base64');
                  handleTelemetry(buf);
                }
              }
            );
          })
          .catch(e => log(`Connection failed: ${e.message}`, 'error'));
      }
    });
  }, [bleManager, log]);

  /**
   * Handle incoming telemetry frames from the device.
   */
  const handleTelemetry = useCallback((buf) => {
    if (buf.length < 1) return;
    const type = buf[0];
    // Parse based on type (matches firmware main.c handle_status)
    if (buf.length >= 8 && type === 0x02) {
      // Status beacon
      setTargetState(['IDLE','DISCOVERY','SWD_CONNECTED','JTAG_CONNECTED',
                       'FLASH_DUMP','LOCKED'][buf[0]] || 'UNKNOWN');
      setBattery(buf[2]);
      log(`Status: ${targetState} battery=${battery}%`);
    } else if (buf.length >= 16) {
      // Discovery result frame
      const proto = ['UNKNOWN','SWD','JTAG','CJTAG','POWER'][buf[0]] || 'UNKNOWN';
      const idcode = buf[1] | (buf[2] << 8) | (buf[3] << 16) | (buf[4] << 24);
      const result = {
        protocol: proto,
        idcode: '0x' + idcode.toString(16).toUpperCase().padStart(8, '0'),
        swdio_ch: buf[5],
        swclk_ch: buf[6],
        tck_ch: buf[7],
        tms_ch: buf[8],
        tdi_ch: buf[9],
        tdo_ch: buf[10],
        gnd_ch: buf[11],
        vref_ch: buf[12],
        vref_mv: buf[13] | (buf[14] << 8),
        confidence: buf[15],
      };
      setDiscovery(result);
      log(`Discovery: ${proto} IDCODE=${result.idcode} conf=${result.confidence}%`);
    }
  }, [targetState, battery, log]);

  /**
   * Send a command to the device.
   */
  const sendCommand = useCallback(async (type, payload = Buffer.alloc(0)) => {
    if (!device) {
      log('Not connected', 'error');
      return;
    }
    const frame = Buffer.concat([Buffer.from([type]), payload]);
    const b64 = frame.toString('base64');
    try {
      await device.writeCharacteristicWithResponseForService(
        GB_SERVICE_UUID, GB_CHAR_COMMAND, b64
      );
      log(`CMD ${type} sent (${payload.length}B payload)`);
    } catch (e) {
      log(`CMD failed: ${e.message}`, 'error');
    }
  }, [device, log]);

  // Initialize service object
  useEffect(() => {
    setService(new GhostBusService(sendCommand, log));
  }, [sendCommand, log]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      bleManager.destroy();
    };
  }, [bleManager]);

  return (
    <NavigationContainer>
      <StatusBar connected={connected} authenticated={authenticated} battery={battery} />
      <Tab.Navigator
        screenOptions={{
          tabBarStyle: { backgroundColor: '#1a1a2e' },
          tabBarActiveTintColor: '#00ff88',
          tabBarInactiveTintColor: '#666',
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#00ff88',
        }}
      >
        <Tab.Screen name="Dashboard">
          {() => <DashboardScreen device={device} connected={connected}
                                   battery={battery} targetState={targetState}
                                   discovery={discovery} connect={connect}
                                   sendCommand={sendCommand} />}
        </Tab.Screen>
        <Tab.Screen name="Discovery">
          {() => <DiscoveryScreen discovery={discovery} sendCommand={sendCommand}
                                  connect={connect} connected={connected} />}
        </Tab.Screen>
        <Tab.Screen name="Memory">
          {() => <MemoryScreen sendCommand={sendCommand} flashDump={flashDump}
                                setFlashDump={setFlashDump} />}
        </Tab.Screen>
        <Tab.Screen name="Console">
          {() => <ConsoleScreen logs={logs} sendCommand={sendCommand} />}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}