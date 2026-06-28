/**
 * App.tsx - Thermal Phantom Companion App
 * 
 * React Native app for controlling the Thermal Phantom device
 * via Bluetooth Low Energy (BLE) or USB serial.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider, DefaultTheme, Text } from 'react-native-paper';
import { BleManager, Device } from 'react-native-ble-plx';

import DashboardScreen from './src/screens/DashboardScreen';
import ProfileEditorScreen from './src/screens/ProfileEditorScreen';
import TriggerConfigScreen from './src/screens/TriggerConfigScreen';
import LaserControlScreen from './src/screens/LaserControlScreen';
import LogViewerScreen from './src/screens/LogViewerScreen';

// ============================================================
// Types
// ============================================================

export interface ThermalPhantomState {
  isConnected: boolean;
  currentTemp: number;
  targetTemp: number;
  mode: ThermalMode;
  tecDuty: number;
  batteryPct: number;
  safetyStatus: string;
  laserArmed: boolean;
  shutterOpen: boolean;
  interlockOk: boolean;
  triggerArmed: boolean;
  profileRunning: boolean;
  profileStep: number;
}

export type ThermalMode = 'idle' | 'heat' | 'cool' | 'laser' | 'profile';

export interface LogEntry {
  timestamp: number;
  level: 'info' | 'warn' | 'error';
  message: string;
}

// ============================================================
// BLE Service Constants
// ============================================================

const THERMAL_PHANTOM_SERVICE = '0000fee1-0000-1000-8000-00805f9b34fb';
const TEMPERATURE_CHAR = '0000fee2-0000-1000-8000-00805f9b34fb';
const COMMAND_CHAR = '0000fee3-0000-1000-8000-00805f9b34fb';
const STATUS_CHAR = '0000fee4-0000-1000-8000-00805f9b34fb';

// ============================================================
// Global BLE Manager and Connection State
// ============================================================

const bleManager = new BleManager();

export const useThermalPhantom = () => {
  const [state, setState] = useState<ThermalPhantomState>({
    isConnected: false,
    currentTemp: 25.0,
    targetTemp: 25.0,
    mode: 'idle',
    tecDuty: 0,
    batteryPct: 100,
    safetyStatus: 'OK',
    laserArmed: false,
    shutterOpen: false,
    interlockOk: false,
    triggerArmed: false,
    profileRunning: false,
    profileStep: 0,
  });

  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [device, setDevice] = useState<Device | null>(null);

  const addLog = useCallback((level: LogEntry['level'], message: string) => {
    setLogs(prev => [...prev.slice(-200), {
      timestamp: Date.now(),
      level,
      message,
    }]);
  }, []);

  const connect = useCallback(async () => {
    addLog('info', 'Scanning for Thermal Phantom...');
    
    bleManager.startDeviceScan(
      [THERMAL_PHANTOM_SERVICE],
      { allowDuplicates: false },
      (error, foundDevice) => {
        if (error) {
          addLog('error', `Scan error: ${error.message}`);
          return;
        }
        if (foundDevice && foundDevice.name?.includes('ThermalPhantom')) {
          bleManager.stopDeviceScan();
          foundDevice.connect()
            .then(d => d.discoverAllServicesAndCharacteristics())
            .then(d => {
              setDevice(d);
              addLog('info', `Connected to ${d.name}`);
              
              // Subscribe to status updates
              d.monitorCharacteristicForService(
                THERMAL_PHANTOM_SERVICE,
                STATUS_CHAR,
                (err, char) => {
                  if (err || !char?.value) return;
                  const decoded = Buffer.from(char.value, 'base64');
                  parseStatusUpdate(decoded);
                }
              );
            })
            .catch(e => addLog('error', `Connection failed: ${e.message}`));
        }
      }
    );
  }, [addLog]);

  const sendCommand = useCallback(async (cmd: number, data: Buffer) => {
    if (!device) return;
    
    const packet = Buffer.alloc(4 + data.length + 2);
    packet.writeUInt8(0xAA, 0);  // Sync
    packet.writeUInt8(cmd, 1);    // Command
    packet.writeUInt16LE(data.length, 2);  // Length
    data.copy(packet, 4);  // Data
    
    // CRC16-CCITT
    const crc = crc16(packet.slice(0, 4 + data.length));
    packet.writeUInt16LE(crc, 4 + data.length);
    
    try {
      await device.writeCharacteristicWithResponseForService(
        THERMAL_PHANTOM_SERVICE,
        COMMAND_CHAR,
        packet.toString('base64')
      );
    } catch (e: any) {
      addLog('error', `Command send failed: ${e.message}`);
    }
  }, [device, addLog]);

  const parseStatusUpdate = useCallback((data: Buffer) => {
    if (data.length < 16) return;
    
    const modeMap: ThermalMode[] = ['idle', 'heat', 'cool', 'laser', 'profile'];
    const modeIdx = data.readUInt8(0);
    
    setState(prev => ({
      ...prev,
      mode: modeMap[modeIdx] || 'idle',
      currentTemp: data.readFloatLE(1),
      targetTemp: data.readFloatLE(5),
      batteryPct: data.readUInt16LE(9),
      tecDuty: data.readInt32LE(11),
      safetyStatus: data.readUInt8(15) ? 'FAULT' : 'OK',
    }));
  }, []);

  const setTargetTemp = useCallback((temp: number) => {
    const buf = Buffer.alloc(2);
    buf.writeInt16LE(Math.round(temp * 10));
    sendCommand(0x01, buf);
    addLog('info', `Set target temperature: ${temp.toFixed(1)}°C`);
  }, [sendCommand, addLog]);

  const setMode = useCallback((mode: ThermalMode) => {
    const modeMap = { idle: 0, heat: 1, cool: 2, laser: 3, profile: 4 };
    const buf = Buffer.alloc(1);
    buf.writeUInt8(modeMap[mode]);
    sendCommand(0x02, buf);
  }, [sendCommand]);

  const armTrigger = useCallback((edge: string, delayUs: number) => {
    const buf = Buffer.alloc(4);
    buf.writeUInt8(1, 0);  // External source
    buf.writeUInt8(edge === 'rising' ? 1 : edge === 'falling' ? 2 : 3, 1);
    buf.writeUInt16LE(delayUs, 2);
    sendCommand(0x03, buf);
    addLog('info', `Trigger armed: edge=${edge}, delay=${delayUs}µs`);
  }, [sendCommand, addLog]);

  const fireLaser = useCallback((powerPct: number, durationMs: number) => {
    const buf = Buffer.alloc(8);
    buf.writeFloatLE(powerPct, 0);
    buf.writeUInt32LE(durationMs, 4);
    sendCommand(0x04, buf);
    addLog('warn', `Laser fired: ${powerPct}% for ${durationMs}ms`);
  }, [sendCommand, addLog]);

  const stop = useCallback(() => {
    sendCommand(0x08, Buffer.alloc(0));
    addLog('error', 'EMERGENCY STOP');
  }, [sendCommand, addLog]);

  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
      setDevice(null);
      addLog('info', 'Disconnected');
    }
    setState(prev => ({ ...prev, isConnected: false }));
  }, [device, addLog]);

  // Auto-connect effect
  useEffect(() => {
    return () => {
      bleManager.destroy();
    };
  }, []);

  return {
    state,
    logs,
    device,
    connect,
    disconnect,
    setTargetTemp,
    setMode,
    armTrigger,
    fireLaser,
    stop,
    addLog,
  };
};

// ============================================================
// CRC16-CCITT
// ============================================================

function crc16(data: Buffer): number {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }
  return crc;
}

// Buffer polyfill for React Native
const Buffer = {
  alloc(size: number): any {
    return {
      _data: new Uint8Array(size),
      _len: size,
      _offset: 0,
      length: size,
      writeUInt8(val: number, offset: number) {
        this._data[offset] = val & 0xFF;
      },
      writeUInt16LE(val: number, offset: number) {
        this._data[offset] = val & 0xFF;
        this._data[offset + 1] = (val >> 8) & 0xFF;
      },
      writeUInt32LE(val: number, offset: number) {
        this._data[offset] = val & 0xFF;
        this._data[offset + 1] = (val >> 8) & 0xFF;
        this._data[offset + 2] = (val >> 16) & 0xFF;
        this._data[offset + 3] = (val >> 24) & 0xFF;
      },
      writeFloatLE(val: number, offset: number) {
        const buf = new ArrayBuffer(4);
        new DataView(buf).setFloat32(0, val, true);
        const view = new Uint8Array(buf);
        for (let i = 0; i < 4; i++) this._data[offset + i] = view[i];
      },
      readUInt8(offset: number) { return this._data[offset]; },
      readUInt16LE(offset: number) {
        return this._data[offset] | (this._data[offset + 1] << 8);
      },
      readFloatLE(offset: number) {
        const buf = new ArrayBuffer(4);
        const view = new Uint8Array(buf);
        for (let i = 0; i < 4; i++) view[i] = this._data[offset + i];
        return new DataView(buf).getFloat32(0, true);
      },
      readInt32LE(offset: number) {
        return (this._data[offset]) |
               (this._data[offset + 1] << 8) |
               (this._data[offset + 2] << 16) |
               (this._data[offset + 3] << 24);
      },
      copy(target: any, targetStart: number) {
        for (let i = 0; i < this._len; i++) {
          target._data[targetStart + i] = this._data[i];
        }
      },
      slice(start: number, end?: number) {
        const len = (end || this._len) - start;
        const newBuf = Buffer.alloc(len);
        for (let i = 0; i < len; i++) {
          newBuf._data[i] = this._data[start + i];
        }
        return newBuf;
      },
      toString(encoding: string) {
        if (encoding === 'base64') {
          let binary = '';
          for (let i = 0; i < this._len; i++) {
            binary += String.fromCharCode(this._data[i]);
          }
          return btoa(binary);
        }
        return '';
      },
    };
  },
  from(base64: string, encoding: string): any {
    if (encoding === 'base64') {
      const binary = atob(base64);
      const buf = Buffer.alloc(binary.length);
      for (let i = 0; i < binary.length; i++) {
        buf._data[i] = binary.charCodeAt(i);
      }
      return buf;
    }
    return Buffer.alloc(0);
  },
};

// ============================================================
// Theme
// ============================================================

const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#FF5722',
    accent: '#FF9800',
    background: '#1a1a2e',
    surface: '#16213e',
    text: '#e0e0e0',
  },
};

// ============================================================
// Main App Component
// ============================================================

const Tab = createBottomTabNavigator();

export default function App() {
  const phantom = useThermalPhantom();

  return (
    <PaperProvider theme={theme}>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarStyle: { backgroundColor: '#16213e' },
            tabBarActiveTintColor: '#FF5722',
            tabBarInactiveTintColor: '#888',
          }}
        >
          <Tab.Screen name="Dashboard">
            {(props) => <DashboardScreen {...props} phantom={phantom} />}
          </Tab.Screen>
          <Tab.Screen name="Profiles">
            {(props) => <ProfileEditorScreen {...props} phantom={phantom} />}
          </Tab.Screen>
          <Tab.Screen name="Trigger">
            {(props) => <TriggerConfigScreen {...props} phantom={phantom} />}
          </Tab.Screen>
          <Tab.Screen name="Laser">
            {(props) => <LaserControlScreen {...props} phantom={phantom} />}
          </Tab.Screen>
          <Tab.Screen name="Logs">
            {(props) => <LogViewerScreen {...props} phantom={phantom} />}
          </Tab.Screen>
        </Tab.Navigator>
      </NavigationContainer>
    </PaperProvider>
  );
}

// Author: jayis1