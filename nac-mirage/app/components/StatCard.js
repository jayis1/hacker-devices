// NAC Mirage StatCard component
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatCard({ label, value, accent }) {
  return (
    <View style={[styles.card, { borderColor: accent || '#3ddc97' }]}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
      <Text style={styles.author}>jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flex: 1,
    minWidth: 140,
    backgroundColor: '#111827',
    borderWidth: 1,
    borderRadius: 12,
    padding: 14,
    margin: 6,
  },
  label: {
    color: '#9ca3af',
    fontSize: 12,
    marginBottom: 8,
  },
  value: {
    color: '#f9fafb',
    fontSize: 22,
    fontWeight: '700',
  },
  author: {
    marginTop: 10,
    color: '#3ddc97',
    fontSize: 11,
  },
});
