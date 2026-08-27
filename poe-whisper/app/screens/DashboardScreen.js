// DashboardScreen.js - PoE Whisper overview screen
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import StatCard from '../components/StatCard';

export default function DashboardScreen({ status }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Inline Power State</Text>
      <View style={styles.grid}>
        <StatCard label="Allocated Power" value={`${status.power} W`} accent="#22d3ee" />
        <StatCard label="Advertised Class" value={status.poeClass} accent="#f59e0b" />
        <StatCard label="Voltage" value={`${status.voltage} V`} accent="#34d399" />
        <StatCard label="Thermal Margin" value={`${status.tempMargin} C`} accent="#f87171" />
      </View>
      <Text style={styles.note}>{status.summary}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 14 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  note: {
    color: '#94a3b8',
    lineHeight: 20,
    backgroundColor: '#0f172a',
    borderRadius: 14,
    padding: 14,
  },
});
