/*
 * tpm-phantom — app/components/StatusBar.js
 * Top status bar showing connection state and capture stats.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatusBar({ connected, capturing, mode, totalTx, tpmTx }) {
  const modeText = mode === 1 ? 'LPC' : mode === 2 ? 'SPI' : 'IDLE';
  return (
    <View style={styles.container}>
      <View style={[styles.indicator, connected ? styles.green : styles.red]} />
      <Text style={styles.text}>
        {connected ? 'Connected' : 'Disconnected'}
      </Text>
      <Text style={styles.separator}>|</Text>
      <Text style={[styles.mode, capturing ? styles.active : styles.inactive]}>
        {capturing ? `● CAPTURING [${modeText}]` : `○ IDLE [${modeText}]`}
      </Text>
      <Text style={styles.separator}>|</Text>
      <Text style={styles.stat}>Total: {totalTx}</Text>
      <Text style={styles.separator}>|</Text>
      <Text style={styles.stat}>TPM: {tpmTx}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 12,
    paddingVertical: 8,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  indicator: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  green: { backgroundColor: '#00ff88' },
  red: { backgroundColor: '#ff3344' },
  text: {
    color: '#e0e0e0',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  separator: {
    color: '#555',
    marginHorizontal: 6,
  },
  mode: {
    fontSize: 12,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  active: { color: '#00ff88' },
  inactive: { color: '#888' },
  stat: {
    color: '#aaccff',
    fontSize: 12,
    fontFamily: 'monospace',
  },
});