/**
 * CAN Creeper Companion App
 *
 * React Native application for controlling the CAN Creeper hardware device.
 * Provides real-time CAN frame monitoring, DBC file parsing, signal graphing,
 * scripted injection sequences, and device configuration over BLE or USB.
 *
 * Navigation: Bottom tabs with Sniffer, Injector, Scripts, and Settings screens.
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

import SnifferScreen from './screens/SnifferScreen';
import InjectorScreen from './screens/InjectorScreen';
import ScriptsScreen from './screens/ScriptsScreen';
import SettingsScreen from './screens/SettingsScreen';
import ConnectionIndicator from './components/ConnectionIndicator';
import ProtocolHandler from './utils/protocol';

const Tab = createBottomTabNavigator();

/* Custom dark theme for CAN Creeper */
const CanCreeperTheme = {
  ...DefaultTheme,
  dark: true,
  colors: {
    ...DefaultTheme.colors,
    primary: '#FF6B00',       /* CAN bus orange */
    background: '#0D1117',    /* Dark background */
    card: '#161B22',          /* Card background */
    text: '#E6EDF3',          /* Light text */
    border: '#30363D',        /* Subtle borders */
    notification: '#FF6B00',
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
    can0FrameCount: 0,
    can1FrameCount: 0,
    can0Tec: 0,
    can0Rec: 0,
    can1Tec: 0,
    can1Rec: 0,
    can0Overflow: 0,
    can1Overflow: 0,
  });
  const [isScanning, setIsScanning] = useState(false);
  const appState = useRef(AppState.currentState);

  /**
   * Handle app state changes (background/foreground)
   */
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (nextAppState) => {
      if (appState.current.match(/inactive|background/) && nextAppState === 'active') {
        /* App came to foreground — reconnect if needed */
        if (!connectionState.bleConnected && !connectionState.usbConnected) {
          protocol.reconnect();
        }
      } else if (nextAppState === 'background') {
        /* App going to background — keep connection alive for logging */
      }
      appState.current = nextAppState;
    });

    return () => {
      subscription.remove();
    };
  }, [connectionState]);

  /**
   * Handle incoming CAN frames from device
   */
  const handleFrameReceived = useCallback((frame) => {
    /* Frames are dispatched to the active screen via event emitter */
    /* The SnifferScreen listens for 'onFrameReceived' events */
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
    protocol.on('statusUpdate', handleStatusUpdate);
    protocol.on('connectionChange', handleConnectionChange);
    protocol.on('error', (error) => {
      Alert.alert('Device Error', `Error code: 0x${error.code.toString(16)}\nData: 0x${error.data.toString(16)}`);
    });

    /* Auto-connect on startup */
    protocol.autoConnect();

    return () => {
      protocol.disconnect();
      protocol.removeAllListeners();
    };
  }, [handleFrameReceived, handleStatusUpdate, handleConnectionChange]);

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
    <NavigationContainer theme={CanCreeperTheme}>
      <StatusBar barStyle="light-content" backgroundColor="#0D1117" />
      <View style={{ flex: 1 }}>
        {/* Connection status bar */}
        <ConnectionIndicator
          bleConnected={connectionState.bleConnected}
          usbConnected={connectionState.usbConnected}
          deviceName={connectionState.deviceName}
          batteryMv={deviceStatus.batteryMv}
          isScanning={isScanning}
          onScanPress={startScan}
        />

        {/* Main tab navigator */}
        <Tab.Navigator
          screenOptions={{
            headerShown: false,
            tabBarActiveTintColor: '#FF6B00',
            tabBarInactiveTintColor: '#8B949E',
            tabBarStyle: {
              backgroundColor: '#161B22',
              borderTopColor: '#30363D',
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
            name="Sniffer"
            options={{
              tabBarLabel: 'Sniffer',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>📡</Text>
              ),
            }}
          >
            {() => (
              <SnifferScreen
                protocol={protocol}
                deviceStatus={deviceStatus}
                connectionState={connectionState}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Injector"
            options={{
              tabBarLabel: 'Injector',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>💉</Text>
              ),
            }}
          >
            {() => (
              <InjectorScreen
                protocol={protocol}
                connectionState={connectionState}
              />
            )}
          </Tab.Screen>

          <Tab.Screen
            name="Scripts"
            options={{
              tabBarLabel: 'Scripts',
              tabBarIcon: ({ color, size }) => (
                <Text style={{ color, fontSize: size }}>📜</Text>
              ),
            }}
          >
            {() => (
              <ScriptsScreen
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

        {/* Scanning overlay */}
        {isScanning && (
          <View style={{
            position: 'absolute',
            top: 40,
            left: 0,
            right: 0,
            backgroundColor: '#161B22',
            padding: 16,
            borderBottomWidth: 1,
            borderBottomColor: '#30363D',
            flexDirection: 'row',
            alignItems: 'center',
            justifyContent: 'center',
          }}>
            <ActivityIndicator color="#FF6B00" size="small" />
            <Text style={{ color: '#E6EDF3', marginLeft: 12, fontSize: 14 }}>
              Scanning for CAN Creeper devices...
            </Text>
          </View>
        )}
      </View>
    </NavigationContainer>
  );
};

export default App;
