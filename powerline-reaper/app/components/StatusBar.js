// components/StatusBar.js — top status bar showing link/mode/battery
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { useReaper } from '../utils/protocol';

export default function StatusBar() {
  const { status, connected } = useReaper();
  const linkColor = status.plcLink ? '#00ffaa' : '#ff5555';
  const connColor = connected ? '#00ffaa' : '#888';
  return (
    <View style={styles.bar}>
      <View style={styles.cell}>
        <Text style={[styles.label, { color: linkColor }]}>
          {status.plcLink ? 'PLC LINK' : 'NO LINK'}
        </Text>
      </View>
      <View style={styles.cell}>
        <Text style={styles.label}>MODE: {status.mode}</Text>
      </View>
      <View style={styles.cell}>
        <Text style={styles.label}>STA: {status.stationCount}</Text>
      </View>
      <View style={styles.cell}>
        <Text style={styles.label}>BAT: {status.batteryPct}%</Text>
      </View>
      <View style={styles.cell}>
        <Text style={[styles.label, { color: connColor }]}>
          {connected ? 'BLE' : 'OFFLINE'}
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  bar: { flexDirection: 'row', backgroundColor: '#111', padding: 6, borderBottomWidth: 1, borderBottomColor: '#222' },
  cell: { flex: 1, alignItems: 'center' },
  label: { color: '#eee', fontSize: 11, fontFamily: 'monospace' },
});