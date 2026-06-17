/**
 * App.js — HDMI-Siphon React Native Companion App
 * Author: jayis1
 * Version: 1.0.0
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Main entry point for the HDMI-Siphon mobile companion application.
 * Provides real-time control and monitoring of the HDMI-Siphon device
 * over WiFi (WebSocket) or BLE.
 *
 * Navigation Structure:
 *   - Dashboard Tab: Live device status, video timing, capture controls
 *   - Capture Tab: Frame capture trigger, interval recording, gallery
 *   - OSD Tab: On-screen overlay text editor
 *   - CEC Tab: CEC bus monitor and command injector
 *   - Settings Tab: Device configuration, EDID management, firmware update
 */

import React, {useEffect, useState, useRef, useCallback} from 'react';
import {
  SafeAreaView,
  StatusBar,
  StyleSheet,
  Text,
  useColorScheme,
} from 'react-native';
import {NavigationContainer} from '@react-navigation/native';
import {createBottomTabNavigator} from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// Screen imports
import DashboardScreen from './screens/DashboardScreen';
import CaptureScreen from './screens/CaptureScreen';
import GalleryScreen from './screens/GalleryScreen';
import OSDScreen from './screens/OSDScreen';
import EDIDScreen from './screens/EDIDScreen';
import CECScreen from './screens/CECScreen';
import CovertScreen from './screens/CovertScreen';
import SettingsScreen from './screens/SettingsScreen';

import {ProtocolClient} from './utils/protocol';

const Tab = createBottomTabNavigator();

const App = () => {
  const isDarkMode = useColorScheme() === 'dark';
  const [deviceStatus, setDeviceStatus] = useState(null);
  const [connected, setConnected] = useState(false);
  const protocolRef = useRef(null);

  // Initialize protocol client and connect to device
  useEffect(() => {
    const protocol = new ProtocolClient({
      onStatus: status => {
        setDeviceStatus(status);
        setConnected(true);
      },
      onDisconnect: () => {
        setConnected(false);
      },
      onError: error => {
        console.warn('Protocol error:', error);
      },
    });

    protocolRef.current = protocol;

    // Attempt connection
    protocol.connect();

    return () => {
      protocol.disconnect();
    };
  }, []);

  const backgroundStyle = {
    backgroundColor: isDarkMode ? '#121212' : '#FFFFFF',
  };

  return (
    <SafeAreaView style={[styles.container, backgroundStyle]}>
      <StatusBar
        barStyle={isDarkMode ? 'light-content' : 'dark-content'}
        backgroundColor={backgroundStyle.backgroundColor}
      />
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({route}) => ({
            tabBarIcon: ({focused, color, size}) => {
              let iconName;
              switch (route.name) {
                case 'Dashboard':
                  iconName = 'monitor-dashboard';
                  break;
                case 'Capture':
                  iconName = 'camera';
                  break;
                case 'Gallery':
                  iconName = 'image-multiple';
                  break;
                case 'OSD':
                  iconName = 'text-box-outline';
                  break;
                case 'EDID':
                  iconName = 'card-text-outline';
                  break;
                case 'CEC':
                  iconName = 'remote';
                  break;
                case 'Covert':
                  iconName = 'incognito';
                  break;
                case 'Settings':
                  iconName = 'cog';
                  break;
                default:
                  iconName = 'help';
              }
              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#E53935',
            tabBarInactiveTintColor: 'gray',
            headerShown: true,
            headerStyle: {
              backgroundColor: '#1A1A2E',
            },
            headerTintColor: '#FFFFFF',
            headerTitleStyle: {
              fontWeight: 'bold',
            },
          })}>
          <Tab.Screen
            name="Dashboard"
            options={{title: 'HDMI-Siphon'}}>
            {props => (
              <DashboardScreen
                {...props}
                status={deviceStatus}
                connected={connected}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="Capture"
            options={{title: 'Capture'}}>
            {props => (
              <CaptureScreen
                {...props}
                status={deviceStatus}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="Gallery"
            options={{title: 'Gallery'}}>
            {props => (
              <GalleryScreen
                {...props}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="OSD"
            options={{title: 'OSD Overlay'}}>
            {props => (
              <OSDScreen
                {...props}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="EDID"
            options={{title: 'EDID Manager'}}>
            {props => (
              <EDIDScreen
                {...props}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="CEC"
            options={{title: 'CEC Console'}}>
            {props => (
              <CECScreen
                {...props}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="Covert"
            options={{title: 'Covert Mode'}}>
            {props => (
              <CovertScreen
                {...props}
                protocol={protocolRef.current}
              />
            )}
          </Tab.Screen>
          <Tab.Screen
            name="Settings"
            options={{title: 'Settings'}}>
            {props => (
              <SettingsScreen
                {...props}
                protocol={protocolRef.current}
                status={deviceStatus}
              />
            )}
          </Tab.Screen>
        </Tab.Navigator>
      </NavigationContainer>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
});

export default App;
