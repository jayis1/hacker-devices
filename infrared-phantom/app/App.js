/**
 * App.js — Infrared Phantom companion app
 * React Native with React Navigation + Paper
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider, DefaultTheme } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import CaptureScreen from './screens/CaptureScreen';
import TransmitScreen from './screens/TransmitScreen';
import AnalyzeScreen from './screens/AnalyzeScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

/* Custom theme matching the dark IR aesthetic */
const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#FF6B35',
    accent: '#FF9F1C',
    background: '#1A1A2E',
    surface: '#16213E',
    text: '#E8E8E8',
    placeholder: '#7B7B7B',
  },
};

export default function App() {
  return (
    <PaperProvider theme={theme}>
      <NavigationContainer theme={theme}>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ color, size }) => {
              let iconName;
              switch (route.name) {
                case 'Capture':
                  iconName = 'access-point';
                  break;
                case 'Transmit':
                  iconName = 'satellite-uplink';
                  break;
                case 'Analyze':
                  iconName = 'chart-bar';
                  break;
                case 'Settings':
                  iconName = 'cog';
                  break;
                default:
                  iconName = 'help-circle';
              }
              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#FF6B35',
            tabBarInactiveTintColor: '#7B7B7B',
            tabBarStyle: { backgroundColor: '#16213E' },
            headerStyle: { backgroundColor: '#16213E' },
            headerTintColor: '#E8E8E8',
          })}
        >
          <Tab.Screen
            name="Capture"
            component={CaptureScreen}
            options={{ title: 'IR Capture' }}
          />
          <Tab.Screen
            name="Transmit"
            component={TransmitScreen}
            options={{ title: 'IR Transmit' }}
          />
          <Tab.Screen
            name="Analyze"
            component={AnalyzeScreen}
            options={{ title: 'IR Analyze' }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </PaperProvider>
  );
}