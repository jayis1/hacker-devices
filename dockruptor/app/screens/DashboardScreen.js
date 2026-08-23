// Dockruptor Dashboard Screen
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';
import StatCard from '../components/StatCard';

export default function DashboardScreen({ session }) {
  const { metrics } = session;

  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Live Session Overview</Text>
      <View style={styles.grid}>
        <StatCard label="Contract" value={metrics.contract} />
        <StatCard label="Host Mode" value={metrics.hostMode} accent="#ffd166" />
        <StatCard label="Battery" value={metrics.battery} accent="#77dd77" />
        <StatCard label="Cable Profile" value={metrics.cable} accent="#ff9aa2" />
        <StatCard label="Relay Bypass" value={metrics.bypass} accent="#7fd1ff" />
        <StatCard label="Board Temp" value={metrics.temperature} accent="#f9a03f" />
      </View>
      <View style={styles.noteCard}>
        <Text style={styles.noteTitle}>Operator Note</Text>
        <Text style={styles.noteBody}>
          Dockruptor is positioned inline and currently using an authorized baseline profile that clamps
          PD capability to 9V/2A, downgrades cable identity, and delays DisplayPort alt-mode release.
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { gap: 16 },
  heading: {
    color: '#e5f1ff',
    fontSize: 22,
    fontWeight: '800',
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
  },
  noteCard: {
    backgroundColor: '#102338',
    borderRadius: 16,
    padding: 16,
  },
  noteTitle: {
    color: '#7fd1ff',
    fontWeight: '800',
    marginBottom: 8,
  },
  noteBody: {
    color: '#aabdd8',
    lineHeight: 22,
  },
});
