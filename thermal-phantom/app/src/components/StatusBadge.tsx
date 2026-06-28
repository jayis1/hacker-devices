/**
 * StatusBadge.tsx - Status indicator badge component
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text } from 'react-native-paper';

interface StatusBadgeProps {
  label: string;
  value: string;
  status?: 'ok' | 'warning' | 'error' | 'neutral';
}

export default function StatusBadge({
  label,
  value,
  status = 'neutral',
}: StatusBadgeProps) {
  const colors = {
    ok: '#4CAF50',
    warning: '#FF9800',
    error: '#F44336',
    neutral: '#2196F3',
  };

  const bgColor = colors[status];

  return (
    <View style={[styles.container, { borderColor: bgColor }]}>
      <View style={[styles.dot, { backgroundColor: bgColor }]} />
      <View style={styles.textContainer}>
        <Text style={styles.label}>{label}</Text>
        <Text style={[styles.value, { color: bgColor }]}>{value}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 10,
    borderRadius: 8,
    borderWidth: 1,
    backgroundColor: '#16213e',
    margin: 4,
    flex: 1,
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  textContainer: {
    flex: 1,
  },
  label: {
    fontSize: 10,
    color: '#888',
    textTransform: 'uppercase',
  },
  value: {
    fontSize: 14,
    fontWeight: 'bold',
  },
});

// Author: jayis1