//==============================================================================
// App.tsx — Spectre-Sniffer Companion Application
// Author: jayis1
// Description: React Native companion app for remote control, data
//              visualization, and post-processing of EM side-channel
//              captures from the Spectre-Sniffer device.
//==============================================================================

import React, { useEffect, useState } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import { StatusBar, LogBox } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// Screen imports
import DashboardScreen from './src/screens/DashboardScreen';
import SpectrumScreen from './src/screens/SpectrumScreen';
import TempestScreen from './src/screens/TempestScreen';
import CryptoScreen from './src/screens/CryptoScreen';
import CaptureScreen from './src/screens/CaptureScreen';
import SettingsScreen from './src/screens/SettingsScreen';

// Services
import { SpectreAPI } from './src/services/api';

// Types
import { DeviceStatus } from './src/types';

LogBox.ignoreLogs(['Reanimated']);

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

//==============================================================================
// Main Tab Navigator
//==============================================================================
const MainTabs: React.FC = () => {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ focused, color, size }) => {
          let iconName: string;

          switch (route.name) {
            case 'Dashboard':
              iconName = 'monitor-dashboard';
              break;
            case 'Spectrum':
              iconName = 'chart-bell-curve';
              break;
            case 'Tempest':
              iconName = 'monitor-screenshot';
              break;
            case 'Crypto':
              iconName = 'lock-outline';
              break;
            case 'Settings':
              iconName = 'cog-outline';
              break;
            default:
              iconName = 'circle';
          }

          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#00E676',
        tabBarInactiveTintColor: '#666',
        tabBarStyle: {
          backgroundColor: '#1a1a2e',
          borderTopColor: '#2a2a4e',
          paddingBottom: 5,
          height: 60,
        },
        headerStyle: {
          backgroundColor: '#1a1a2e',
          elevation: 0,
          shadowOpacity: 0,
        },
        headerTintColor: '#fff',
        headerTitleStyle: {
          fontWeight: 'bold',
          fontSize: 18,
        },
      })}
    >
      <Tab.Screen name="Dashboard" component={DashboardScreen} />
      <Tab.Screen name="Spectrum" component={SpectrumScreen} />
      <Tab.Screen name="Tempest" component={TempestScreen} />
      <Tab.Screen name="Crypto" component={CryptoScreen} />
      <Tab.Screen name="Settings" component={SettingsScreen} />
    </Tab.Navigator>
  );
};

//==============================================================================
// Root App Component
//==============================================================================
const App: React.FC = () => {
  const [deviceStatus, setDeviceStatus] = useState<DeviceStatus | null>(null);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    // Poll device status every 5 seconds
    const interval = setInterval(async () => {
      try {
        const status = await SpectreAPI.getStatus();
        setDeviceStatus(status);
        setConnected(true);
      } catch (error) {
        setConnected(false);
      }
    }, 5000);

    return () => clearInterval(interval);
  }, []);

  return (
    <NavigationContainer
      theme={{
        dark: true,
        colors: {
          primary: '#00E676',
          background: '#0f0f23',
          card: '#1a1a2e',
          text: '#ffffff',
          border: '#2a2a4e',
          notification: '#ff4444',
        },
      }}
    >
      <StatusBar barStyle="light-content" backgroundColor="#0f0f23" />
      <Stack.Navigator screenOptions={{ headerShown: false }}>
        <Stack.Screen name="Main" component={MainTabs} />
        <Stack.Screen
          name="Capture"
          component={CaptureScreen}
          options={{
            headerShown: true,
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#fff',
            title: 'Capture Manager',
          }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
};

export default App;
