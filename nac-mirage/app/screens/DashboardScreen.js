// NAC Mirage dashboard screen
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import StatCard from '../components/StatCard';

export default function DashboardScreen({ status }) {
  return (
    <View>
      <Text style={styles.title}>NAC Mirage Live Overview</Text>
      <Text style={styles.subtitle}>Inline PoE / LLDP / NAC bridge telemetry by jayis1.</Text>
      <View style={styles.row}>
        <StatCard label="Profile" value={status.profile} accent="#3b82f6" />
        <StatCard label="Frames Seen" value={String(status.framesSeen)} accent="#22c55e" />
      </View>
      <View style={styles.row}>
        <StatCard label="Mutated" value={String(status.framesMutated)} accent="#f59e0b" />
        <StatCard label="Dropped" value={String(status.framesDropped)} accent="#ef4444" />
      </View>
      <View style={styles.row}>
        <StatCard label="Advertised VLAN" value={String(status.advertisedVlan)} accent="#8b5cf6" />
        <StatCard label="PoE Budget" value={`${status.poeBudgetMw} mW`} accent="#14b8a6" />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  title: {
    color: '#f9fafb',
    fontSize: 24,
    fontWeight: '800',
    marginBottom: 8,
  },
  subtitle: {
    color: '#9ca3af',
    marginBottom: 16,
  },
  row: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
});
