/**
 * StatusIndicator.js — Reusable status indicator component
 * Author: jayis1
 * Date:   2026-06-17
 *
 * A simple labeled status row with a colored indicator dot.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatusIndicator({ label, value, status = 'neutral' }) {
  const colors = {
    good: '#3fb950',
    error: '#f85149',
    warning: '#d29922',
    pending: '#d29922',
    neutral: '#8b949e',
  };

  const dotColor = colors[status] || colors.neutral;

  return (
    <View style={styles.container}>
      <View style={[styles.dot, { backgroundColor: dotColor }]} />
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 6,
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  label: {
    color: '#8b949e',
    fontSize: 14,
    flex: 1,
  },
  value: {
    color: '#e6edf3',
    fontSize: 14,
    fontWeight: '500',
    textAlign: 'right',
  },
});