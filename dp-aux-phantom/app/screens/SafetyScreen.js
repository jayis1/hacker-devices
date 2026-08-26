// DP AUX Phantom safety screen
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SafetyScreen({ checklist }) {
  return (
    <View style={styles.wrapper}>
      <Text style={styles.title}>Authorized-use checklist</Text>
      <Text style={styles.subtitle}>
        DP AUX Phantom is intended for display security validation, interoperability research, and red-team lab exercises on systems you own or are permitted to test.
      </Text>
      {checklist.map((item) => (
        <View key={item} style={styles.item}>
          <Text style={styles.bullet}>•</Text>
          <Text style={styles.text}>{item}</Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    gap: 14,
  },
  title: {
    color: '#f8fafc',
    fontSize: 22,
    fontWeight: '800',
  },
  subtitle: {
    color: '#94a3b8',
    lineHeight: 20,
  },
  item: {
    flexDirection: 'row',
    gap: 10,
    alignItems: 'flex-start',
    backgroundColor: '#111827',
    borderWidth: 1,
    borderColor: '#1f2937',
    borderRadius: 16,
    padding: 14,
  },
  bullet: {
    color: '#22d3ee',
    fontSize: 18,
    fontWeight: '900',
  },
  text: {
    color: '#e5e7eb',
    flex: 1,
    lineHeight: 20,
  },
});
