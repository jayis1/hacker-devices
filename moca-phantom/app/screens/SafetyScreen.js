// MoCA Phantom safety screen
// Author: jayis1

import React from 'react';
import { StyleSheet, Switch, Text, View } from 'react-native';

export default function SafetyScreen({ safeMode, onToggleSafeMode }) {
  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Safety and Authorization</Text>
      <View style={styles.card}>
        <Text style={styles.title}>Authorized use only</Text>
        <Text style={styles.body}>
          MoCA Phantom is for explicit, written, authorized security research and defensive validation only.
          Do not connect it to hospitality, residential, provider, or shared coax infrastructure without permission.
        </Text>
      </View>
      <View style={styles.toggleRow}>
        <View style={{ flex: 1 }}>
          <Text style={styles.title}>Safe Mode</Text>
          <Text style={styles.body}>Keep bounded manipulations disabled unless the engagement plan explicitly allows active validation.</Text>
        </View>
        <Switch value={safeMode} onValueChange={onToggleSafeMode} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1 },
  heading: {
    color: '#edf4ff',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  card: {
    backgroundColor: '#0f1d31',
    borderRadius: 14,
    padding: 16,
    marginBottom: 18,
  },
  toggleRow: {
    backgroundColor: '#0f1d31',
    borderRadius: 14,
    padding: 16,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 16,
  },
  title: {
    color: '#edf4ff',
    fontWeight: '700',
    marginBottom: 6,
  },
  body: {
    color: '#9cb4d2',
    lineHeight: 20,
  },
});
