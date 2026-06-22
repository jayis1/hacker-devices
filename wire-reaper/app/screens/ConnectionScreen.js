/**
 * ConnectionScreen.js — BLE device scanning and connection
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet, ActivityIndicator } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { useDevice } from '../components/DeviceContext';

export default function ConnectionScreen() {
  const { scanForDevices, connectToDevice, status } = useDevice();
  const navigation = useNavigation();
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connecting, setConnecting] = useState(false);

  const handleScan = async () => {
    setScanning(true);
    try {
      const found = await scanForDevices();
      setDevices(found);
    } catch (e) {
      setDevices([]);
    }
    setScanning(false);
  };

  const handleConnect = async (deviceId) => {
    setConnecting(true);
    const ok = await connectToDevice(deviceId);
    setConnecting(false);
    if (ok) {
      navigation.navigate('Dashboard');
    }
  };

  useEffect(() => {
    handleScan();
  }, []);

  const renderDevice = ({ item }) => (
    <TouchableOpacity style={styles.deviceItem} onPress={() => handleConnect(item.id)}>
      <View>
        <Text style={styles.deviceName}>{item.name}</Text>
        <Text style={styles.deviceId}>{item.id}</Text>
      </View>
      <Text style={styles.rssi}>{item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>WireReaper</Text>
        <Text style={styles.subtitle}>Multi-Bus Infiltrator by jayis1</Text>
      </View>

      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠️ For authorized security research only. Use only on systems
          you own or have explicit written permission to test.
        </Text>
      </View>

      <Text style={styles.status}>Status: {status}</Text>

      {scanning && (
        <View style={styles.scanning}>
          <ActivityIndicator size="large" color="#e94560" />
          <Text style={styles.scanningText}>Scanning for WireReaper devices...</Text>
        </View>
      )}

      {connecting && (
        <View style={styles.scanning}>
          <ActivityIndicator size="large" color="#e94560" />
          <Text style={styles.scanningText}>Connecting...</Text>
        </View>
      )}

      <FlatList
        data={devices}
        keyExtractor={item => item.id}
        renderItem={renderDevice}
        style={styles.list}
        ListEmptyComponent={
          !scanning ? (
            <Text style={styles.empty}>No devices found. Tap Scan to retry.</Text>
          ) : null
        }
      />

      <TouchableOpacity style={styles.scanButton} onPress={handleScan} disabled={scanning}>
        <Text style={styles.scanButtonText}>Scan for Devices</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 20 },
  header: { alignItems: 'center', marginTop: 20, marginBottom: 20 },
  title: { fontSize: 28, fontWeight: 'bold', color: '#e94560' },
  subtitle: { fontSize: 14, color: '#888', marginTop: 4 },
  disclaimer: { backgroundColor: '#2a1a1a', borderRadius: 8, padding: 12, marginBottom: 20 },
  disclaimerText: { color: '#ff6b6b', fontSize: 12, textAlign: 'center' },
  status: { color: '#aaa', fontSize: 14, marginBottom: 10 },
  scanning: { alignItems: 'center', padding: 20 },
  scanningText: { color: '#888', marginTop: 10 },
  list: { flex: 1 },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    padding: 16,
    borderRadius: 8,
    marginBottom: 8,
  },
  deviceName: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  deviceId: { color: '#666', fontSize: 12, marginTop: 4 },
  rssi: { color: '#e94560', fontSize: 14 },
  empty: { color: '#666', textAlign: 'center', padding: 20 },
  scanButton: {
    backgroundColor: '#e94560',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 20,
  },
  scanButtonText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
});