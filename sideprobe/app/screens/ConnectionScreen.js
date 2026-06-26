/**
 * ConnectionScreen.js — BLE scan & connect
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ConnectionScreen({ navigation }) {
  const { scanForDevices, connectToDevice, connected, status } = useDevice();
  const [devices, setDevices] = useState([]);
  const [scanning, setScanning] = useState(false);

  const handleScan = useCallback(async () => {
    setScanning(true);
    try {
      const found = await scanForDevices();
      setDevices(found);
    } catch (e) {
      Alert.alert('Scan failed', e.message);
    }
    setScanning(false);
  }, [scanForDevices]);

  const handleConnect = useCallback(async (dev) => {
    const ok = await connectToDevice(dev.id);
    if (ok) navigation.navigate('Dashboard');
  }, [connectToDevice, navigation]);

  useEffect(() => {
    if (connected) navigation.navigate('Dashboard');
  }, [connected, navigation]);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>SideProbe</Text>
      <Text style={styles.subtitle}>Power & EM Side-Channel Cryptanalysis</Text>
      <Text style={styles.author}>by jayis1 — authorized use only</Text>

      <Text style={styles.status}>Status: {status}</Text>

      <TouchableOpacity style={styles.button} onPress={handleScan} disabled={scanning}>
        <Text style={styles.buttonText}>{scanning ? 'Scanning...' : 'Scan for SideProbe'}</Text>
      </TouchableOpacity>

      <FlatList
        data={devices}
        keyExtractor={(item) => item.id}
        renderItem={({ item }) => (
          <TouchableOpacity style={styles.deviceItem} onPress={() => handleConnect(item)}>
            <Text style={styles.deviceName}>{item.name || 'SideProbe'}</Text>
            <Text style={styles.deviceId}>{item.id}</Text>
            <Text style={styles.deviceRssi}>RSSI: {item.rssi} dBm</Text>
          </TouchableOpacity>
        )}
        ListEmptyComponent={<Text style={styles.empty}>No devices found. Tap scan.</Text>}
      />

      <Text style={styles.disclaimer}>
        ⚠️ Legal: Use only on hardware you own or are authorized to assess.
        Recovering keys from unauthorized targets may be illegal.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0d1117' },
  title: { fontSize: 28, fontWeight: 'bold', color: '#ff6b35', marginTop: 20 },
  subtitle: { fontSize: 14, color: '#8b949e', marginBottom: 4 },
  author: { fontSize: 12, color: '#6e7681', marginBottom: 20 },
  status: { color: '#58a6ff', fontSize: 14, marginBottom: 16 },
  button: { backgroundColor: '#ff6b35', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  buttonText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  deviceItem: { backgroundColor: '#161b22', padding: 14, borderRadius: 8, marginBottom: 8, borderWidth: 1, borderColor: '#30363d' },
  deviceName: { color: '#e6edf3', fontSize: 16, fontWeight: 'bold' },
  deviceId: { color: '#8b949e', fontSize: 12 },
  deviceRssi: { color: '#6e7681', fontSize: 12 },
  empty: { color: '#6e7681', textAlign: 'center', padding: 20 },
  disclaimer: { color: '#f0883e', fontSize: 11, marginTop: 16, lineHeight: 16 },
});