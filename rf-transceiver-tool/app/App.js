/**
 * App.js — RF Transceiver Tool Companion App
 *
 * Main React Native application for controlling the RF Transceiver Tool
 * via USB CDC serial connection. Provides device connection, configuration,
 * real-time status monitoring, and packet capture display.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { SafeAreaView, StatusBar, StyleSheet, Text, View, Platform } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import DeviceScreen from './screens/DeviceScreen';
import SettingsScreen from './screens/SettingsScreen';
import { CMD, MODE, MODE_NAMES, buildPacket, parsePacket } from './utils/protocol';

const Stack = createStackNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator
        initialRouteName="Device"
        screenOptions={{
          headerStyle: {
            backgroundColor: '#1a1a2e',
          },
          headerTintColor: '#00ff88',
          headerTitleStyle: {
            fontWeight: 'bold',
            fontSize: 18,
          },
        }}
      >
        <Stack.Screen
          name="Device"
          component={DeviceScreen}
          options={{ title: 'RF Transceiver Tool' }}
        />
        <Stack.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ title: 'Configuration' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
});