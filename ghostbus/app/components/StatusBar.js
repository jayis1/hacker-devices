/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — StatusBar Component
 *
 * Persistent top bar showing BLE connection, authentication, and battery.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export function StatusBar({ connected, authenticated, battery }) {
  return (
    <View style={styles.container}>
      <View style={styles.section}>
        <Text style={[styles.dot, { backgroundColor: connected ? '#00ff88' : '#ff3333' }]} />
        <Text style={styles.label}>BLE {connected ? 'OK' : 'OFF'}</Text>
      </View>
      <View style={styles.section}>
        <Text style={[styles.dot, { backgroundColor: authenticated ? '#00aaff' : '#555' }]} />
        <Text style={styles.label}>AUTH {authenticated ? 'YES' : 'NO'}</Text>
      </View>
      <View style={styles.section}>
        <Text style={styles.label}>🔋 {battery}%</Text>
      </View>
      <View style={styles.sectionRight}>
        <Text style={styles.title}>GHOSTBUS</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flexDirection: 'row', alignItems: 'center',
               backgroundColor: '#1a1a2e', paddingVertical: 8,
               paddingHorizontal: 16, borderBottomWidth: 1,
               borderBottomColor: '#00ff88' },
  section: { flexDirection: 'row', alignItems: 'center', marginRight: 16 },
  sectionRight: { marginLeft: 'auto' },
  dot: { width: 8, height: 8, borderRadius: 4, marginRight: 6 },
  label: { color: '#aaa', fontSize: 12, fontFamily: 'monospace' },
  title: { color: '#00ff88', fontSize: 14, fontWeight: 'bold',
           fontFamily: 'monospace', letterSpacing: 2 },
});