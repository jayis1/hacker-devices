// src/components/StatusCard.tsx — Status display card component
//
// Author: jayis1
// License: GPL-2.0

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

interface StatusCardProps {
  label: string;
  value: string;
  warning?: boolean;
}

export default function StatusCard({ label, value, warning }: StatusCardProps) {
  return (
    <View style={[styles.card, warning && styles.cardWarning]}>
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, warning && styles.valueWarning]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1a1a1a',
    borderRadius: 8,
    padding: 12,
    marginBottom: 6,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  cardWarning: {
    borderColor: '#ff8800',
    backgroundColor: '#2a1a00',
  },
  label: {
    fontSize: 13,
    color: '#888',
  },
  value: {
    fontSize: 13,
    color: '#ccc',
    fontFamily: 'monospace',
    fontWeight: '600',
  },
  valueWarning: {
    color: '#ff8800',
  },
});

// Author: jayis1
// License: GPL-2.0