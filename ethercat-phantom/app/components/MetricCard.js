// MetricCard component for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function MetricCard({ label, value, accent }) {
  return (
    <View style={[styles.card, { borderColor: accent || '#3cb1ff' }]}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
      <Text style={styles.author}>jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#101725',
    borderWidth: 1,
    borderRadius: 14,
    padding: 14,
    marginBottom: 12,
    minWidth: 150,
  },
  label: {
    color: '#7f91ad',
    fontSize: 13,
    marginBottom: 6,
  },
  value: {
    color: '#ecf4ff',
    fontSize: 20,
    fontWeight: '700',
  },
  author: {
    color: '#4b5d79',
    marginTop: 8,
    fontSize: 10,
  },
});
