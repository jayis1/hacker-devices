// MoCA Phantom stat card component
// Author: jayis1

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

export default function StatCard({ label, value, accent = '#28c0f0' }) {
  return (
    <View style={[styles.card, { borderColor: accent }]}> 
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, { color: accent }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#0f1d31',
    borderWidth: 1,
    borderRadius: 14,
    padding: 14,
    minWidth: 150,
    gap: 6,
  },
  label: {
    color: '#92a8c4',
    fontSize: 12,
    textTransform: 'uppercase',
    letterSpacing: 0.8,
  },
  value: {
    fontSize: 22,
    fontWeight: '700',
  },
});
