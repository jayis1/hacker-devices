// StatCard.js - PoE Whisper companion UI component
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatCard({ label, value, accent }) {
  return (
    <View style={[styles.card, accent && { borderColor: accent }]}>
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, accent && { color: accent }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#0f172a',
    borderRadius: 14,
    padding: 14,
    borderWidth: 1,
    borderColor: '#1e293b',
    minWidth: '47%',
  },
  label: {
    color: '#94a3b8',
    fontSize: 12,
    marginBottom: 6,
    textTransform: 'uppercase',
  },
  value: {
    color: '#f8fafc',
    fontSize: 20,
    fontWeight: '800',
  },
});
