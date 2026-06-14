/**
 * StatusIndicator.js — ShadowTap reusable status indicator component
 * Displays a labeled circle that changes color based on active state.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatusIndicator({ label, active, color = '#00ff88' }) {
  return (
    <View style={styles.container}>
      <View
        style={[
          styles.dot,
          {
            backgroundColor: active ? color : '#333',
            shadowColor: active ? color : 'transparent',
            shadowOpacity: active ? 0.6 : 0,
            shadowRadius: 8,
            shadowOffset: { width: 0, height: 0 },
          },
        ]}
      />
      <Text style={[styles.label, { color: active ? color : '#555' }]}>
        {label}
      </Text>
      <Text style={styles.status}>{active ? 'UP' : 'DOWN'}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    padding: 10,
  },
  dot: {
    width: 16,
    height: 16,
    borderRadius: 8,
    marginBottom: 6,
    elevation: 4,
  },
  label: {
    fontSize: 12,
    fontWeight: 'bold',
    letterSpacing: 1,
  },
  status: {
    fontSize: 10,
    color: '#555',
    marginTop: 2,
  },
});