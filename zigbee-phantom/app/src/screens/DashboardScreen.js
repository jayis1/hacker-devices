// src/screens/DashboardScreen.js — Device status overview
// Author: jayis1
// License: MIT
import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { useDevice } from '../context/DeviceContext';

const MODE_NAMES = ['SNIFF', 'KEY-HUNT', 'ROGUE COORD', 'JAM', 'RELAY'];

export default function DashboardScreen() {
  const { connected, connect, status, commands } = useDevice();

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>ZIGBEE-PHANTOM</Text>
        <Text style={styles.subtitle}>by jayis1</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>
        <Text style={styles.value}>
          Status: <Text style={{color: connected ? '#4caf50' : '#f44336'}}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </Text>
        {!connected && (
          <TouchableOpacity style={styles.button} onPress={connect}>
            <Text style={styles.buttonText}>Connect via BLE</Text>
          </TouchableOpacity>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Radio Status</Text>
        <Text style={styles.value}>Mode: {MODE_NAMES[status.mode] || '—'}</Text>
        <Text style={styles.value}>Channel: {status.channel} ({2405 + 5*status.channel} MHz)</Text>
        <Text style={styles.value}>RSSI: {status.rssi} dBm</Text>
        <Text style={styles.value}>Battery: {status.battPct}%</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Capture Counters</Text>
        <Text style={styles.value}>Frames captured: {status.frameCount}</Text>
        <Text style={styles.value}>Transport-Key frames: {status.keyCount}</Text>
        <Text style={styles.value}>Frames injected: {status.injectCount}</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Actions</Text>
        <View style={styles.row}>
          <TouchableOpacity style={styles.qbtn} onPress={commands.energyScan}>
            <Text style={styles.qbtnText}>Energy Scan</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.qbtn} onPress={commands.getStatus}>
            <Text style={styles.qbtnText}>Refresh Status</Text>
          </TouchableOpacity>
        </View>
      </View>

      <Text style={styles.disclaimer}>
        ⚠ Authorized security research only. Obtain written consent before use.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 16 },
  header: { alignItems: 'center', marginBottom: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#e85d2c' },
  subtitle: { fontSize: 12, color: '#888' },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginBottom: 12 },
  cardTitle: { fontSize: 14, fontWeight: 'bold', color: '#e85d2c', marginBottom: 8 },
  value: { color: '#ddd', fontSize: 14, marginBottom: 4 },
  button: { backgroundColor: '#e85d2c', padding: 12, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  row: { flexDirection: 'row', gap: 8, marginTop: 8 },
  qbtn: { flex: 1, backgroundColor: '#2a2a4e', padding: 10, borderRadius: 6, alignItems: 'center' },
  qbtnText: { color: '#e85d2c', fontSize: 12, fontWeight: 'bold' },
  disclaimer: { color: '#f44336', fontSize: 10, marginTop: 16, textAlign: 'center' },
});