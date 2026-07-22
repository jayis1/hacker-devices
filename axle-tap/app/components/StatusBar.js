// StatusBar.js — persistent status banner for AxleTap app
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatusBar({ connected, armed, captureRunning, bleConnected }) {
  return (
    <View style={styles.container}>
      <View style={[styles.dot, connected ? styles.green : styles.red]} />
      <Text style={styles.text}>Link {connected ? 'UP' : 'DOWN'}</Text>
      <View style={[styles.dot, bleConnected ? styles.green : styles.gray]} />
      <Text style={styles.text}>BLE {bleConnected ? 'OK' : '--'}</Text>
      <View style={[styles.dot, captureRunning ? styles.blue : styles.gray]} />
      <Text style={styles.text}>CAP {captureRunning ? 'REC' : 'idle'}</Text>
      {armed && (
        <View style={styles.armBanner}>
          <Text style={styles.armText}>⚠ ARMED — TRANSMIT ENABLED</Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 10,
    backgroundColor: '#1a1a1a',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 4,
  },
  green:  { backgroundColor: '#00ff66' },
  red:    { backgroundColor: '#ff3344' },
  blue:   { backgroundColor: '#3399ff' },
  gray:   { backgroundColor: '#666' },
  text: {
    color: '#eee',
    fontSize: 12,
    marginRight: 14,
    fontFamily: 'monospace',
  },
  armBanner: {
    backgroundColor: '#ff3344',
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 4,
    marginLeft: 'auto',
  },
  armText: {
    color: '#fff',
    fontSize: 11,
    fontWeight: 'bold',
  },
});