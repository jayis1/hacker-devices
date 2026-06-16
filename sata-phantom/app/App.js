/**
 * App.js — SATA Phantom Companion App
 * Author: jayis1
 *
 * Root component that initializes navigation, theme, and global state.
 */

import React, { useEffect, useState } from 'react';
import { StatusBar, LogBox } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import AppNavigator from './src/navigation/AppNavigator';

// Suppress known warnings in dev
LogBox.ignoreLogs(['Non-serializable values']);

const App = () => {
  return (
    <SafeAreaProvider>
      <StatusBar barStyle="light-content" backgroundColor="#0a0a1a" />
      <NavigationContainer
        theme={{
          dark: true,
          colors: {
            primary: '#00ff88',
            background: '#0a0a1a',
            card: '#12122a',
            text: '#e0e0e0',
            border: '#2a2a4a',
            notification: '#ff4444',
          },
        }}
      >
        <AppNavigator />
      </NavigationContainer>
    </SafeAreaProvider>
  );
};

export default App;
