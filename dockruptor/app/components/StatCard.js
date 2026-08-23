// Dockruptor Stat Card
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

export default function StatCard({ label, value, accent = '#7fd1ff' }) {
  return (
    <View style={styles.card}>
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, { color: accent }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#102338',
    borderRadius: 16,
    padding: 16,
    minWidth: 150,
    flex: 1,
  },
  label: {
    color: '#93aac8',
    fontSize: 12,
    textTransform: 'uppercase',
    letterSpacing: 0.6,
    marginBottom: 8,
  },
  value: {
    color: '#ffffff',
    fontSize: 20,
    fontWeight: '800',
  },
});
