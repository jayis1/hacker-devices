// DP AUX Phantom stat card
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatCard({ label, value, hint }) {
  return (
    <View style={styles.card}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
      {hint ? <Text style={styles.hint}>{hint}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#111827',
    borderRadius: 18,
    padding: 16,
    borderWidth: 1,
    borderColor: '#1f2937',
    gap: 8,
    minWidth: 150,
  },
  label: {
    color: '#9ca3af',
    fontSize: 12,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  value: {
    color: '#f9fafb',
    fontSize: 26,
    fontWeight: '800',
  },
  hint: {
    color: '#67e8f9',
    lineHeight: 18,
  },
});
