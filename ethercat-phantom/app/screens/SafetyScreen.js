// Safety screen for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SafetyScreen() {
  const checks = [
    'Target authorization confirmed',
    'Failsafe bypass relay continuity verified',
    'Rules bounded to process-safe objects only',
    'Time synchronization within acceptance window',
    'Data retention policy approved by client',
  ];

  return (
    <View>
      <Text style={styles.sectionTitle}>Safety & Engagement Controls</Text>
      {checks.map((item) => (
        <View key={item} style={styles.checkRow}>
          <Text style={styles.checkBullet}>•</Text>
          <Text style={styles.checkText}>{item}</Text>
        </View>
      ))}
      <Text style={styles.copy}>Author: jayis1. For legal, authorized security validation only.</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  sectionTitle: {
    color: '#eaf3fe',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  checkRow: {
    flexDirection: 'row',
    marginBottom: 8,
  },
  checkBullet: {
    color: '#53b2ff',
    width: 16,
    fontSize: 16,
  },
  checkText: {
    color: '#d7e4f5',
    flex: 1,
  },
  copy: {
    color: '#94a9c3',
    marginTop: 14,
    lineHeight: 21,
  },
});
