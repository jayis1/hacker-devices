/**
 * DashboardScreen.js — Main dashboard showing device status
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, RefreshControl } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { useDevice } from '../components/DeviceContext';
import { CMD, parseStatus } from '../utils/protocol';
import { Buffer } from 'buffer';

export default function DashboardScreen() {
  const { sendCommand, connected, status, disconnect } = useDevice();
  const navigation = useNavigation();
  const [deviceStatus, setDeviceStatus] = useState(null);
  const [refreshing, setRefreshing] = useState(false);

  const fetchStatus = useCallback(async () => {
    const resp = await sendCommand(CMD.STATUS);
    if (resp && resp.length >= 20) {
      setDeviceStatus(parseStatus(resp));
    }
  }, [sendCommand]);

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 3000);
    return () => clearInterval(interval);
  }, [fetchStatus]);

  const onRefresh = async () => {
    setRefreshing(true);
    await fetchStatus();
    setRefreshing(false);
  };

  const handleDisconnect = async () => {
    await disconnect();
    navigation.navigate('Connection');
  };

  const menuItems = [
    { title: 'Live Capture', screen: 'CaptureView', icon: '📡', desc: 'Real-time bus traffic' },
    { title: 'I²C Console', screen: 'I2cConsole', icon: '🔌', desc: 'Scan, read, write, emulate' },
    { title: 'SPI Console', screen: 'SpiConsole', icon: '💾', desc: 'Flash dump, register access' },
    { title: 'UART Terminal', screen: 'UartTerminal', icon: '🖥️', desc: 'Debug console access' },
    { title: '1-Wire Tools', screen: 'OneWire', icon: '🔑', desc: 'iButton / DS24xx read' },
    { title: 'Replay Manager', screen: 'ReplayManager', icon: '🔄', desc: 'Capture replay attacks' },
    { title: 'Bus Fuzzer', screen: 'Fuzzer', icon: '🧪', desc: 'Autonomous fuzzing' },
  ];

  return (
    <ScrollView
      style={styles.container}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} tintColor="#e94560" />}
    >
      {/* Status header */}
      <View style={styles.statusHeader}>
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Connection:</Text>
          <Text style={[styles.statusValue, { color: connected ? '#4ecca3' : '#e94560' }]}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Battery:</Text>
          <Text style={styles.statusValue}>{deviceStatus ? `${deviceStatus.battery}%` : '---'}</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Uptime:</Text>
          <Text style={styles.statusValue}>{deviceStatus ? `${(deviceStatus.uptime / 1000).toFixed(0)}s` : '---'}</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Capturing:</Text>
          <Text style={[styles.statusValue, { color: deviceStatus?.capturing ? '#e94560' : '#888' }]}>
            {deviceStatus?.capturing ? 'YES' : 'No'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Records:</Text>
          <Text style={styles.statusValue}>{deviceStatus ? deviceStatus.captureCount.toLocaleString() : '0'}</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Fuzzing:</Text>
          <Text style={[styles.statusValue, { color: deviceStatus?.fuzzActive ? '#e94560' : '#888' }]}>
            {deviceStatus?.fuzzActive ? `ACTIVE (${deviceStatus.fuzzIterations})` : 'No'}
          </Text>
        </View>
      </View>

      {/* Menu grid */}
      <View style={styles.menuGrid}>
        {menuItems.map((item, index) => (
          <TouchableOpacity
            key={index}
            style={styles.menuCard}
            onPress={() => navigation.navigate(item.screen)}
          >
            <Text style={styles.menuIcon}>{item.icon}</Text>
            <Text style={styles.menuTitle}>{item.title}</Text>
            <Text style={styles.menuDesc}>{item.desc}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <TouchableOpacity style={styles.disconnectButton} onPress={handleDisconnect}>
        <Text style={styles.disconnectText}>Disconnect</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>WireReaper v1.0 — by jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  statusHeader: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
  },
  statusRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  statusLabel: { color: '#888', fontSize: 14 },
  statusValue: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  menuGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  menuCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    width: '48%',
    marginBottom: 12,
    alignItems: 'center',
  },
  menuIcon: { fontSize: 32, marginBottom: 8 },
  menuTitle: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 4 },
  menuDesc: { color: '#666', fontSize: 11, textAlign: 'center' },
  disconnectButton: {
    backgroundColor: '#2a1a1a',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
    marginBottom: 12,
  },
  disconnectText: { color: '#ff6b6b', fontWeight: 'bold' },
  footer: { color: '#444', textAlign: 'center', fontSize: 12, marginBottom: 20 },
});