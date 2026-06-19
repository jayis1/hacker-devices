// components/SignalChart.js — simple ASCII-style SNR bar chart
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SignalChart({ stations }) {
  const max = Math.max(80, ...stations.map((s) => s.snr));
  return (
    <View style={styles.container}>
      {stations.map((s, i) => {
        const pct = (s.snr / max) * 100;
        const bar = '█'.repeat(Math.max(1, Math.round(pct / 4)));
        return (
          <View key={i} style={styles.row}>
            <Text style={styles.mac}>{s.mac.slice(-8)}</Text>
            <Text style={styles.bar}>{bar}</Text>
            <Text style={styles.val}>{s.snr}dB</Text>
          </View>
        );
      })}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { padding: 8 },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 4 },
  mac: { color: '#888', fontSize: 10, fontFamily: 'monospace', width: 80 },
  bar: { color: '#00ffaa', fontSize: 10, fontFamily: 'monospace' },
  val: { color: '#ffaa00', fontSize: 10, fontFamily: 'monospace', marginLeft: 'auto' },
});