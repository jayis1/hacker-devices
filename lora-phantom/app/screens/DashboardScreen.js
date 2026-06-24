/*
 * lora-phantom / app / screens / DashboardScreen.js
 * Device overview: battery, RF state, mode, capture rate, SD free.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, MODES, REGION_NAMES } from '../utils/protocol';

export default function DashboardScreen({ navigation }) {
  const { connected, status, frames, joinReqs, sendCommand } = useDevice();

  useEffect(() => {
    if (connected) {
      const interval = setInterval(() => sendCommand(CMD.STATUS), 3000);
      return () => clearInterval(interval);
    }
  }, [connected, sendCommand]);

  const modeNames = ['IDLE', 'SNIFF', 'KEYSEARCH', 'REPLAY', 'INJECT', 'GATEWAY_EMU', 'FUZZ', 'SCAN'];

  const navItems = [
    { label: 'Sniffer', screen: 'Sniffer', icon: '📡' },
    { label: 'Key Search', screen: 'KeySearch', icon: '🔑' },
    { label: 'Replay Manager', screen: 'Replay', icon: '🔄' },
    { label: 'Injector', screen: 'Injector', icon: '📤' },
    { label: 'Gateway Emulator', screen: 'GatewayEmu', icon: '🏠' },
    { label: 'PHY Fuzzer', screen: 'Fuzzer', icon: '🧪' },
    { label: 'Spectrum Scan', screen: 'SpectrumScan', icon: '📊' },
    { label: 'Settings', screen: 'Settings', icon: '⚙️' },
  ];

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Dashboard</Text>

      {/* Status Cards */}
      <View style={styles.row}>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>Battery</Text>
          <Text style={styles.cardValue}>{status ? status.batMv : '—'} mV</Text>
          <Text style={styles.cardSub}>{status ? Math.round((status.batMv - 3300) / 9) + '%' : ''}</Text>
        </View>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>Mode</Text>
          <Text style={styles.cardValue}>{status ? modeNames[status.mode] : '—'}</Text>
        </View>
      </View>

      <View style={styles.row}>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>Region</Text>
          <Text style={styles.cardValue}>{status ? REGION_NAMES[status.region] : '—'}</Text>
        </View>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>TX Power</Text>
          <Text style={styles.cardValue}>{status ? status.txPower : '—'} dBm</Text>
        </View>
      </View>

      <View style={styles.row}>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>Sniff Freq</Text>
          <Text style={styles.cardValue}>{status ? (status.sniffFreq / 1e6).toFixed(1) : '—'} MHz</Text>
          <Text style={styles.cardSub}>SF{status?.sniffSf} BW{status?.sniffBw === 0 ? 125 : status?.sniffBw === 1 ? 250 : 500}</Text>
        </View>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>Frames</Text>
          <Text style={styles.cardValue}>{status ? status.frameCount : 0}</Text>
        </View>
      </View>

      <View style={styles.row}>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>SD Free</Text>
          <Text style={styles.cardValue}>{status ? Math.round(status.sdFreeKb / 1024) : '—'} MB</Text>
        </View>
        <View style={styles.card}>
          <Text style={styles.cardLabel}>Uptime</Text>
          <Text style={styles.cardValue}>{status ? Math.round(status.uptimeMs / 1000) : '—'} s</Text>
        </View>
      </View>

      {/* Quick Actions */}
      <Text style={styles.section}>Quick Actions</Text>
      <View style={styles.row}>
        <TouchableOpacity style={styles.actionBtn}
          onPress={() => sendCommand(CMD.SET_MODE, new Uint8Array([MODES.SNIFF]))}>
          <Text style={styles.actionText}>Start Sniff</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn}
          onPress={() => sendCommand(CMD.SET_MODE, new Uint8Array([MODES.IDLE]))}>
          <Text style={styles.actionText}>Stop</Text>
        </TouchableOpacity>
      </View>

      {/* Navigation Grid */}
      <Text style={styles.section}>Tools</Text>
      <View style={styles.grid}>
        {navItems.map(item => (
          <TouchableOpacity key={item.screen} style={styles.navItem}
            onPress={() => navigation.navigate(item.screen)}>
            <Text style={styles.navIcon}>{item.icon}</Text>
            <Text style={styles.navLabel}>{item.label}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Recent Activity */}
      <Text style={styles.section}>Recent Activity</Text>
      <Text style={styles.activityText}>Frames captured: {frames.length}</Text>
      <Text style={styles.activityText}>Join-requests: {joinReqs.length}</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00ff88', marginBottom: 15 },
  row: { flexDirection: 'row', gap: 10, marginBottom: 10 },
  card: { flex: 1, backgroundColor: '#1a1a1a', borderRadius: 8, padding: 12,
          borderWidth: 1, borderColor: '#333' },
  cardLabel: { color: '#888', fontSize: 12 },
  cardValue: { color: '#fff', fontSize: 20, fontWeight: 'bold', marginTop: 4 },
  cardSub: { color: '#666', fontSize: 11, marginTop: 2 },
  section: { fontSize: 16, fontWeight: 'bold', color: '#ccc', marginTop: 20, marginBottom: 10 },
  actionBtn: { flex: 1, backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
               borderRadius: 6, padding: 12, alignItems: 'center' },
  actionText: { color: '#00ff88', fontSize: 14, fontWeight: '600' },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  navItem: { width: '47%', backgroundColor: '#1a1a1a', borderRadius: 8, padding: 15,
             borderWidth: 1, borderColor: '#333', alignItems: 'center' },
  navIcon: { fontSize: 24, marginBottom: 5 },
  navLabel: { color: '#ccc', fontSize: 13, fontWeight: '600' },
  activityText: { color: '#aaa', fontSize: 13, marginBottom: 3 },
});