/**
 * GLYPH-REAPER Companion App
 *
 * React Native application for controlling the GLYPH-REAPER hardware device.
 * Provides live display mirroring of captured screen content, text extraction
 * with credential detection, frame-by-frame analysis, and device configuration.
 *
 * Navigation: Bottom tabs with Live View, Capture, Text, and Analysis screens.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  NavigationContainer,
  DefaultTheme,
} from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import {
  StatusBar,
  Text,
  View,
  ActivityIndicator,
  Alert,
  AppState,
} from 'react-native';

import LiveViewScreen from './screens/LiveViewScreen';
import CaptureScreen from './screens/CaptureScreen';
import TextExtractionScreen from './screens/TextExtractionScreen';
import FrameAnalysisScreen from './screens/FrameAnalysisScreen';
import SettingsScreen from './screens/SettingsScreen';
import ConnectionIndicator from './components/ConnectionIndicator';
import ProtocolHandler from './utils/protocol';

const Tab = createBottomTabNavigator();

/* Custom dark theme for GLYPH-REAPER */
const GlyphReaperTheme = {
  ...DefaultTheme,
  dark: true,
  colors: {
    ...DefaultTheme.colors,
    primary: '#00D4AA',       /* Display green */
    background: '#0A0E14',    /* Near-black */
    card: '#131720',          /* Card background */
    text: '#E6EDF3',          /* Light text */
    border: '#1E242C',        /* Subtle borders */
    notification: '#FF4757',  /* Alert red */
  },
};

/* Global protocol handler instance */
const protocol = new ProtocolHandler();

/**
 * Main App component
 */
const App = () => {
  const [connectionState, setConnectionState] = useState({
    bleConnected: false,
    usbConnected: false,
    deviceName: '',
    deviceId: '',
  });
  const [deviceStatus, setDeviceStatus] = useState({
    uptime: 0,
    batteryMv: 0,
    temperature: 0,
    systemState: 'IDLE',
    protocol: 'NONE',
    frameWidth: 0,
    frameHeight: 0,
    colorDepth: 0,
    captureFps: 0,
    totalFrames: 0,
    droppedFrames: 0,
    compressionRatio: 0,
    ocrTextsExtracted: 0,
    credentialAlerts: 0,
  });
  const [isScanning, setIsScanning] = useState(false);
  const appState = useRef(AppState.currentState);

  /**
   * Handle app state changes (background/foreground)
   */
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (nextAppState) => {
      if (appState.current.match(/inactive|background/) && nextAppState === 'active') {
        if (!connectionState.bleConnected && !connectionState.usbConnected) {
          protocol.reconnect();
        }
      }
      appState.current = nextAppState;
    });

    return () => {
      subscription.remove();
    };
  }, [connectionState]);

  /**
   * Handle incoming frame data from device
   */
  const handleFrameReceived = useCallback((frame) => {
    /* Frames are dispatched to LiveViewScreen via event emitter */
  }, []);

  /**
   * Handle OCR text results
   */
  const handleOcrText = useCallback((text) => {
    /* Dispatched to TextExtractionScreen */
  }, []);

  /**
   * Handle credential alerts
   */
  const handleCredentialAlert = useCallback((alert) => {
    /* Show notification for detected credentials */
    if (alert.patternType === 'JWT_TOKEN' || 
        alert.patternType === 'PASSWORD_FIELD' ||
        alert.patternType === 'API_KEY') {
      Alert.alert(
        '⚠ Credential Detected',
        `Pattern: ${alert.patternType}\nText: ${alert.text.substring(0, 50)}...\nFrame: ${alert.frameIndex}`,
        [{ text: 'OK' }]
      );
    }
  }, []);

  /**
   * Handle device status updates
   */
  const handleStatusUpdate = useCallback((status) => {
    setDeviceStatus(status);
  }, []);

  /**
   * Handle connection state changes
   */
  const handleConnectionChange = useCallback((state) => {
    setConnectionState(state);
  }, []);

  /**
   * Initialize protocol handler
   */
  useEffect(() => {
    protocol.on('frameReceived', handleFrameReceived);
    protocol.on('ocrText', handleOcrText);
    protocol.on('credentialAlert', handleCredentialAlert);
    protocol.on('statusUpdate', handleStatusUpdate);
    protocol.on('connectionChange', handleConnectionChange);
    protocol.on('error', (error) => {
      Alert.alert('Device Error', `Code: 0x${error.code.toString(16)}\n${error.message}`);
    });

    protocol.autoConnect();

    return () => {
      protocol.disconnect();
      protocol.removeAllListeners();
    };
  }, [handleFrameReceived, handleOcrText, handleCredentialAlert, 
      handleStatusUpdate, handleConnectionChange]);

  /**
   * Start BLE scan
   */
  const startScan = useCallback(async () => {
    setIsScanning(true);
    try {
      await protocol.startBLEScan();
    } catch (error) {
      Alert.alert('Scan Error', error.message);
    } finally {
      setIsScanning(false);
    }
  }, []);

  /**
   * Connect to a specific device
   */
  const connectToDevice = useCallback(async (deviceId) => {
    try {
      await protocol.connectBLE(deviceId);
    } catch (error) {
      Alert.alert('Connection Error', error.message);
    }
  }, []);

  return (
    <NavigationContainer theme={GlyphReaperTheme}>
      <StatusBar barStyle="light-content" backgroundColor="#0A0E14" />
      <View style={{ flex: 1 }}>
        <ConnectionIndicator
          bleConnected={connectionState.bleConnected}
          usbConnected={connectionState.usbConnected}
          deviceName={connectionState.deviceName}
          batteryMv={deviceStatus.batteryMv}
          isScanning={isScanning}
          onScanPress={startScan}
        />

        <Tab.Navigator
          screenOptions={{
            headerShown: false,
            tabBarActiveTintColor: '#00D4AA',
            tabBarInactiveTintColor: '#5C6877',
            tabBarStyle: {
              backgroundColor: '#131720',
              borderTopColor: '#1E242C',
              borderTopWidth: 1,
              height: 60,
              paddingBottom: 8,
              paddingTop: 4,
            },
            tabBarLabelStyle: {
              fontSize: 11,
              fontWeight: '600',
            },
          }}
        >
          <Tab.Screen
            name="LiveView"
            options={{
              tabBarLabel: 'Live View',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>🖥️</Text>
              ),
            }}
          >
            {() => (
              <LiveViewScreen
                protocol={protocol}
                deviceStatus={deviceStatus}
                connectionState={connectionState}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Capture"
            options={{
              tabBarLabel: 'Capture',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>🎬</Text>
              ),
            }}
          >
            {() => (
              <CaptureScreen
                protocol={protocol}
                deviceStatus={deviceStatus}
                connectionState={connectionState}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Text"
            options={{
              tabBarLabel: 'Text',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>📝</Text>
              ),
            }}
          >
            {() => (
              <TextExtractionScreen
                protocol={protocol}
                connectionState={connectionState}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Analysis"
            options={{
              tabBarLabel: 'Analysis',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>🔍</Text>
              ),
            }}
          >
            {() => (
              <FrameAnalysisScreen
                protocol={protocol}
                connectionState={connectionState}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Settings"
            options={{
              tabBarLabel: 'Settings',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>⚙️</Text>
              ),
            }}
          >
            {() => (
              <SettingsScreen
                protocol={protocol}
                connectionState={connectionState}
                deviceStatus={deviceStatus}
                onConnectDevice={connectToDevice}
              />
            )}
          </Tab.Screen>
        </Tab.Navigator>

        {isScanning && (
          <View style={{
            position: 'absolute',
            top: 40,
            left: 0,
            right: 0,
            backgroundColor: '#131720',
            padding: 16,
            borderBottomWidth: 1,
            borderBottomColor: '#1E242C',
            flexDirection: 'row',
            alignItems: 'center',
            justifyContent: 'center',
          }}>
            <ActivityIndicator color="#00D4AA" size="small" />
            <Text style={{ color: '#E6EDF3', marginLeft: 12, fontSize: 14 }}>
              Scanning for GLYPH-REAPER devices...
            </Text>
          </View>
        )}
      </View>
    </NavigationContainer>
  );
};

export default App;