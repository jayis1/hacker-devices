/**
 * App.js — PCIe Screamer Companion App Entry Point
 *
 * React Native application for controlling the PCIe Screamer
 * NVMe interposer and analyzing captured PCIe traffic.
 *
 * Navigation structure:
 *   - CaptureScreen: Real-time TLP stream, filter controls, buffer status
 *   - AnalysisScreen: TLP decode, NVMe command extraction, search
 *   - InjectionScreen: TLP injection builder, DMA attack simulation
 *   - SettingsScreen: Device configuration, firmware update, export
 *
 * Author: jayis1
 * Date: 2026-06-16
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  NavigationContainer,
  DefaultTheme,
} from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import {
  View,
  Text,
  StyleSheet,
  StatusBar,
  ActivityIndicator,
  Alert,
  AppState,
} from 'react-native';

import CaptureScreen from './screens/CaptureScreen';
import AnalysisScreen from './screens/AnalysisScreen';
import InjectionScreen from './screens/InjectionScreen';
import SettingsScreen from './screens/SettingsScreen';
import DeviceContext, { DeviceProvider } from './components/DeviceContext';
import { ScreamerProtocol } from './utils/protocol';

const Tab = createBottomTabNavigator();

/* Custom dark theme for the app */
const ScreamerTheme = {
  ...DefaultTheme,
  dark: true,
  colors: {
    ...DefaultTheme.colors,
    primary: '#00FF41',       /* Matrix green */
    background: '#0A0E17',    /* Dark navy */
    card: '#111827',          /* Slightly lighter navy */
    text: '#E5E7EB',          /* Light gray */
    border: '#1F2937',        /* Dark border */
    notification: '#EF4444',  /* Red for alerts */
  },
};

/**
 * Main App component with device connection management
 */
const App = () => {
  const [deviceState, setDeviceState] = useState({
    connected: false,
    connecting: false,
    status: null,
    error: null,
    protocol: null,
  });

  const protocolRef = useRef(null);
  const appStateRef = useRef(AppState.currentState);

  /* Initialize protocol handler */
  useEffect(() => {
    protocolRef.current = new ScreamerProtocol();
    return () => {
      if (protocolRef.current) {
        protocolRef.current.disconnect();
      }
    };
  }, []);

  /* Handle app state changes (background/foreground) */
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (nextAppState) => {
      if (appStateRef.current.match(/active/) && nextAppState.match(/inactive|background/)) {
        /* App going to background — pause streaming */
        if (protocolRef.current && deviceState.connected) {
          protocolRef.current.pauseStreaming();
        }
      } else if (appStateRef.current.match(/inactive|background/) && nextAppState === 'active') {
        /* App returning to foreground — resume streaming */
        if (protocolRef.current && deviceState.connected) {
          protocolRef.current.resumeStreaming();
        }
      }
      appStateRef.current = nextAppState;
    });

    return () => subscription.remove();
  }, [deviceState.connected]);

  /* Connect to device */
  const connectToDevice = useCallback(async () => {
    setDeviceState(prev => ({ ...prev, connecting: true, error: null }));

    try {
      const protocol = protocolRef.current;
      await protocol.connect();

      /* Query initial status */
      const status = await protocol.queryStatus();

      setDeviceState({
        connected: true,
        connecting: false,
        status,
        error: null,
        protocol,
      });
    } catch (err) {
      setDeviceState({
        connected: false,
        connecting: false,
        status: null,
        error: err.message || 'Connection failed',
        protocol: null,
      });
      Alert.alert('Connection Error', err.message || 'Failed to connect to PCIe Screamer');
    }
  }, []);

  /* Disconnect from device */
  const disconnectFromDevice = useCallback(async () => {
    try {
      if (protocolRef.current) {
        await protocolRef.current.disconnect();
      }
    } catch (err) {
      /* Ignore disconnect errors */
    }
    setDeviceState({
      connected: false,
      connecting: false,
      status: null,
      error: null,
      protocol: null,
    });
  }, []);

  /* Update device status periodically */
  useEffect(() => {
    if (!deviceState.connected) return;

    const interval = setInterval(async () => {
      try {
        if (protocolRef.current) {
          const status = await protocolRef.current.queryStatus();
          setDeviceState(prev => ({ ...prev, status }));
        }
      } catch (err) {
        /* Status poll failed — device may have disconnected */
        setDeviceState(prev => ({
          ...prev,
          error: 'Status poll failed. Device may be disconnected.',
        }));
      }
    }, 2000); /* Poll every 2 seconds */

    return () => clearInterval(interval);
  }, [deviceState.connected]);

  /* Connection screen shown when not connected */
  if (!deviceState.connected) {
    return (
      <View style={styles.connectContainer}>
        <StatusBar barStyle="light-content" backgroundColor="#0A0E17" />
        <Text style={styles.title}>PCIe Screamer</Text>
        <Text style={styles.subtitle}>NVMe Interposer & Traffic Analyzer</Text>

        {deviceState.connecting ? (
          <View style={styles.connectingBox}>
            <ActivityIndicator size="large" color="#00FF41" />
            <Text style={styles.connectingText}>Connecting to device...</Text>
            <Text style={styles.hintText}>Ensure USB 3.0 cable is connected</Text>
          </View>
        ) : (
          <View style={styles.connectBox}>
            <Text style={styles.connectPrompt}>
              Connect the PCIe Screamer via USB 3.0 to begin
            </Text>
            <TouchableButton
              title="Connect"
              onPress={connectToDevice}
              color="#00FF41"
            />
            {deviceState.error && (
              <Text style={styles.errorText}>{deviceState.error}</Text>
            )}
          </View>
        )}

        <Text style={styles.versionText}>v1.0.0 — jayis1</Text>
      </View>
    );
  }

  /* Main app with tab navigation */
  return (
    <DeviceProvider value={{
      deviceState,
      setDeviceState,
      protocol: protocolRef.current,
      connect: connectToDevice,
      disconnect: disconnectFromDevice,
    }}>
      <NavigationContainer theme={ScreamerTheme}>
        <StatusBar barStyle="light-content" backgroundColor="#111827" />
        <Tab.Navigator
          screenOptions={{
            tabBarActiveTintColor: '#00FF41',
            tabBarInactiveTintColor: '#6B7280',
            tabBarStyle: {
              backgroundColor: '#111827',
              borderTopColor: '#1F2937',
              borderTopWidth: 1,
            },
            headerStyle: {
              backgroundColor: '#111827',
            },
            headerTintColor: '#E5E7EB',
            headerTitleStyle: {
              fontWeight: 'bold',
            },
          }}
        >
          <Tab.Screen
            name="Capture"
            component={CaptureScreen}
            options={{
              tabBarLabel: 'Capture',
              headerTitle: 'TLP Capture',
              tabBarIcon: ({ color }) => (
                <Text style={{ color, fontSize: 20 }}>◉</Text>
              ),
            }}
          />
          <Tab.Screen
            name="Analysis"
            component={AnalysisScreen}
            options={{
              tabBarLabel: 'Analysis',
              headerTitle: 'TLP Analysis',
              tabBarIcon: ({ color }) => (
                <Text style={{ color, fontSize: 20 }}>▣</Text>
              ),
            }}
          />
          <Tab.Screen
            name="Injection"
            component={InjectionScreen}
            options={{
              tabBarLabel: 'Injection',
              headerTitle: 'TLP Injection',
              tabBarIcon: ({ color }) => (
                <Text style={{ color, fontSize: 20 }}>▶</Text>
              ),
            }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{
              tabBarLabel: 'Settings',
              headerTitle: 'Device Settings',
              tabBarIcon: ({ color }) => (
                <Text style={{ color, fontSize: 20 }}>⚙</Text>
              ),
            }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
};

/* Simple touchable button component */
const TouchableButton = ({ title, onPress, color }) => (
  <View style={styles.buttonContainer}>
    <Text
      style={[styles.button, { backgroundColor: color }]}
      onPress={onPress}
    >
      {title}
    </Text>
  </View>
);

const styles = StyleSheet.create({
  connectContainer: {
    flex: 1,
    backgroundColor: '#0A0E17',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  title: {
    fontSize: 32,
    fontWeight: 'bold',
    color: '#00FF41',
    marginBottom: 8,
    fontFamily: 'monospace',
  },
  subtitle: {
    fontSize: 16,
    color: '#9CA3AF',
    marginBottom: 40,
    fontFamily: 'monospace',
  },
  connectBox: {
    alignItems: 'center',
    padding: 20,
  },
  connectPrompt: {
    fontSize: 14,
    color: '#D1D5DB',
    textAlign: 'center',
    marginBottom: 20,
  },
  connectingBox: {
    alignItems: 'center',
    padding: 20,
  },
  connectingText: {
    fontSize: 16,
    color: '#00FF41',
    marginTop: 16,
    fontFamily: 'monospace',
  },
  hintText: {
    fontSize: 12,
    color: '#6B7280',
    marginTop: 8,
  },
  buttonContainer: {
    marginVertical: 8,
  },
  button: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#0A0E17',
    paddingHorizontal: 32,
    paddingVertical: 12,
    borderRadius: 4,
    overflow: 'hidden',
    fontFamily: 'monospace',
  },
  errorText: {
    fontSize: 12,
    color: '#EF4444',
    marginTop: 12,
    textAlign: 'center',
  },
  versionText: {
    fontSize: 10,
    color: '#4B5563',
    position: 'absolute',
    bottom: 20,
    fontFamily: 'monospace',
  },
});

export default App;
