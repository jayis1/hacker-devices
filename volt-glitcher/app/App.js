/**
 * App.js — VoltGlitcher Pro companion application
 * React Native app for configuring and monitoring voltage glitch attacks
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StyleSheet, View, Text, StatusBar, Platform } from 'react-native';

import ControlScreen from './screens/ControlScreen';
import AnalysisScreen from './screens/AnalysisScreen';
import GlitchConfig from './components/GlitchConfig';
import { VoltGlitcherProtocol, DEVICE_STATES } from './utils/protocol';

const Tab = createBottomTabNavigator();

/* ============================================================================
 * Theme Constants
 * ============================================================================ */

const COLORS = {
  background:    '#0a0a0f',
  surface:       '#1a1a2e',
  surfaceLight:  '#252540',
  primary:       '#6c5ce7',
  primaryDark:   '#4834d4',
  accent:        '#00cec9',
  danger:        '#ff6b6b',
  warning:       '#ffeaa7',
  success:       '#55efc4',
  textPrimary:   '#ffffff',
  textSecondary: '#a0a0b0',
  border:        '#2d2d44',
};

/* ============================================================================
 * Device Context — shared state between screens
 * ============================================================================ */

export const DeviceContext = React.createContext(null);

function App() {
  const [deviceState, setDeviceState] = useState(DEVICE_STATES.DISCONNECTED);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [glitchConfig, setGlitchConfig] = useState(null);
  const [results, setResults] = useState(null);
  const [events, setEvents] = useState([]);
  const [connectionType, setConnectionType] = useState('usb'); // 'usb' | 'ble'
  const protocolRef = useRef(null);
  const eventBufferRef = useRef([]);

  /* Initialize protocol handler */
  useEffect(() => {
    const protocol = new VoltGlitcherProtocol();
    protocolRef.current = protocol;

    protocol.onStateChange = (state) => {
      setDeviceState(state);
    };

    protocol.onDeviceInfo = (info) => {
      setDeviceInfo(info);
    };

    protocol.onResults = (newResults) => {
      setResults(newResults);
    };

    protocol.onEvent = (event) => {
      eventBufferRef.current.push(event);
      if (eventBufferRef.current.length > 1000) {
        eventBufferRef.current = eventBufferRef.current.slice(-500);
      }
      setEvents([...eventBufferRef.current]);
    };

    protocol.onConfigUpdate = (config) => {
      setGlitchConfig(config);
    };

    return () => {
      protocol.disconnect();
    };
  }, []);

  /* Auto-connect on mount */
  useEffect(() => {
    const connect = async () => {
      if (protocolRef.current) {
        try {
          await protocolRef.current.connect();
        } catch (e) {
          console.warn('Auto-connect failed:', e.message);
        }
      }
    };
    connect();
  }, []);

  /* Action handlers passed to child screens */
  const actions = useCallback({
    connect: async () => {
      try {
        await protocolRef.current.connect();
      } catch (e) {
        console.error('Connect failed:', e);
      }
    },

    disconnect: async () => {
      await protocolRef.current.disconnect();
    },

    arm: async () => {
      try {
        await protocolRef.current.arm();
        setDeviceState(DEVICE_STATES.ARMED);
      } catch (e) {
        console.error('Arm failed:', e);
      }
    },

    disarm: async () => {
      try {
        await protocolRef.current.disarm();
        setDeviceState(DEVICE_STATES.IDLE);
      } catch (e) {
        console.error('Disarm failed:', e);
      }
    },

    fire: async () => {
      try {
        await protocolRef.current.fireManual();
      } catch (e) {
        console.error('Fire failed:', e);
      }
    },

    updateConfig: async (newConfig) => {
      try {
        await protocolRef.current.setGlitchConfig(newConfig);
        setGlitchConfig({ ...glitchConfig, ...newConfig });
      } catch (e) {
        console.error('Config update failed:', e);
      }
    },

    loadProfile: async (profileId) => {
      try {
        const config = await protocolRef.current.loadProfile(profileId);
        setGlitchConfig(config);
      } catch (e) {
        console.error('Profile load failed:', e);
      }
    },

    saveProfile: async (profileId, name) => {
      try {
        await protocolRef.current.saveProfile(profileId, name, glitchConfig);
      } catch (e) {
        console.error('Profile save failed:', e);
      }
    },

    calibrate: async () => {
      try {
        setDeviceState(DEVICE_STATES.CALIBRATING);
        await protocolRef.current.calibrate();
        setDeviceState(DEVICE_STATES.IDLE);
      } catch (e) {
        console.error('Calibration failed:', e);
        setDeviceState(DEVICE_STATES.ERROR);
      }
    },

    markSuccess: async () => {
      try {
        await protocolRef.current.markSuccess();
      } catch (e) {
        console.error('Mark success failed:', e);
      }
    },

    resetDevice: async () => {
      try {
        await protocolRef.current.reset();
        setDeviceState(DEVICE_STATES.IDLE);
        setResults(null);
        setEvents([]);
      } catch (e) {
        console.error('Reset failed:', e);
      }
    },

    readADC: async (channel) => {
      try {
        return await protocolRef.current.readADC(channel);
      } catch (e) {
        console.error('ADC read failed:', e);
        return null;
      }
    },
  }, [glitchConfig]);

  /* Context value for child components */
  const deviceContextValue = {
    deviceState,
    deviceInfo,
    glitchConfig,
    results,
    events,
    connectionType,
    actions,
    colors: COLORS,
  };

  return (
    <DeviceContext.Provider value={deviceContextValue}>
      <SafeAreaView style={styles.container}>
        <StatusBar barStyle="light-content" backgroundColor={COLORS.background} />

        {/* Header bar */}
        <View style={styles.header}>
          <Text style={styles.headerTitle}>⚡ VoltGlitcher Pro</Text>
          <View style={[styles.statusDot, {
            backgroundColor:
              deviceState === DEVICE_STATES.CONNECTED ? COLORS.success :
              deviceState === DEVICE_STATES.ARMED ? COLORS.warning :
              deviceState === DEVICE_STATES.FIRING ? COLORS.danger :
              deviceState === DEVICE_STATES.ERROR ? COLORS.danger :
              COLORS.textSecondary
          }]} />
          <Text style={styles.statusText}>
            {deviceState === DEVICE_STATES.DISCONNECTED ? 'Disconnected' :
             deviceState === DEVICE_STATES.CONNECTED ? 'Connected' :
             deviceState === DEVICE_STATES.IDLE ? 'Idle' :
             deviceState === DEVICE_STATES.ARMED ? 'ARMED' :
             deviceState === DEVICE_STATES.FIRING ? 'FIRING' :
             deviceState === DEVICE_STATES.CALIBRATING ? 'Calibrating...' :
             deviceState === DEVICE_STATES.ERROR ? 'ERROR' : 'Unknown'}
          </Text>
        </View>

        {/* Tab navigation */}
        <NavigationContainer independent={true}>
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: {
                backgroundColor: COLORS.surface,
                borderTopColor: COLORS.border,
              },
              tabBarActiveTintColor: COLORS.primary,
              tabBarInactiveTintColor: COLORS.textSecondary,
              headerShown: false,
            }}
          >
            <Tab.Screen
              name="Control"
              component={ControlScreen}
              options={{
                tabBarLabel: 'Control',
                tabBarIcon: () => <Text>🎮</Text>,
              }}
            />
            <Tab.Screen
              name="Config"
              component={GlitchConfigScreen}
              options={{
                tabBarLabel: 'Config',
                tabBarIcon: () => <Text>⚙️</Text>,
              }}
            />
            <Tab.Screen
              name="Analysis"
              component={AnalysisScreen}
              options={{
                tabBarLabel: 'Analysis',
                tabBarIcon: () => <Text>📊</Text>,
              }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaView>
    </DeviceContext.Provider>
  );
}

/* Wrapper for GlitchConfig as a tab screen */
function GlitchConfigScreen() {
  return (
    <View style={{ flex: 1 }}>
      <GlitchConfig />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0f',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 12,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  headerTitle: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: '700',
    flex: 1,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  statusText: {
    color: '#a0a0b0',
    fontSize: 14,
  },
});

export default App;