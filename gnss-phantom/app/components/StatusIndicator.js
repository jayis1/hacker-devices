/*
 * components/StatusIndicator.js — Reusable status LED indicator
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const COLORS = {
  green:  '#22c55e',
  red:    '#ef4444',
  yellow: '#eab308',
  blue:   '#3b82f6',
  gray:   '#6b7280',
  orange: '#f97316',
};

export default function StatusIndicator({ label, color = 'gray', active = false, size = 12 }) {
  const bgColor = active ? COLORS[color] || COLORS.gray : '#1a1a2e';
  const borderColor = COLORS[color] || COLORS.gray;

  return (
    <View style={styles.container}>
      <View
        style={[
          styles.led,
          {
            width: size,
            height: size,
            borderRadius: size / 2,
            backgroundColor: bgColor,
            borderColor: borderColor,
            borderWidth: 1,
            shadowColor: borderColor,
            shadowOpacity: active ? 0.8 : 0.2,
            shadowRadius: active ? size / 2 : 0,
          },
        ]}
      />
      <Text style={[styles.label, { color: active ? borderColor : '#888' }]}>
        {label}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    marginVertical: 4,
    marginHorizontal: 8,
  },
  led: {
    marginRight: 8,
  },
  label: {
    fontSize: 12,
    fontFamily: 'monospace',
  },
});