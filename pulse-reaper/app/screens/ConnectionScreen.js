/**
 * ConnectionScreen.js — BLE scan + connect
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ConnectionScreen({ navigation }) {
  const { scanForDevices, connectToDevice, connected, status } = useDevice();
  const [devices, setDevices] = useState([]);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    if (connected) {
      navigation.navigate('Dashboard');
    }
  }, [connected, navigation]);

  const handleScan = async () => {
    setScanning(true);
    try {
      const found = await scanForDevices();
      setDevices(found);
    } catch (e) {
      Alert.alert('Scan failed', e.message);
    }
    setScanning(false);
  };

  const handleConnect = async (deviceId) => {
    const ok = await connectToDevice(deviceId);
    if (!ok) {
      Alert.alert('Connection failed', 'Could not connect to device');
    }
  };

  const renderItem = ({ item }) => (
    <TouchableOpacity style={styles.deviceItem} onPress={() => handleConnect(item.id)}>
      <Text style={styles.deviceName}>{item.name || 'Unknown Pulse-Reaper'}</Text>
      <Text style={styles.deviceId}>{item.id}</Text>
      <Text style={styles.deviceRssi}>RSSI: {item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Pulse-Reaper</Text>
      <Text style={styles.subtitle}>TDR-Guided Non-Invasive Covert Tap</Text>
      <Text style={styles.author}>by jayis1</Text>

      <Text style={styles.status}>Status: {status}</Text>

      <TouchableOpacity style={styles.button} onPress={handleScan} disabled={scanning}>
        <Text style={styles.buttonText}>{scanning ? 'Scanning...' : 'Scan for Devices'}</Text>
      </TouchableOpacity>

      <FlatList
        data={devices}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
        style={styles.list}
        ListEmptyComponent={
          <Text style={styles.empty}>No devices found. Tap "Scan for Devices".</Text>
        }
      />

      <Text style={styles.disclaimer}>
        ⚠️ For authorized security research only. See README for legal disclaimer.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0d1117' },
  title: { fontSize: 28, fontWeight: 'bold', color: '#ff6b35', textAlign: 'center', marginTop: 20 },
  subtitle: { fontSize: 14, color: '#8b949e', textAlign: 'center', marginTop: 4 },
  author: { fontSize: 12, color: '#6e7681', textAlign: 'center', marginTop: 2 },
  status: { fontSize: 14, color: '#00d4aa', marginTop: 20, marginBottom: 10 },
  button: { backgroundColor: '#ff6b35', padding: 14, borderRadius: 8, alignItems: 'center', marginVertical: 10 },
  buttonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  list: { flex: 1, marginTop: 10 },
  deviceItem: { backgroundColor: '#161b22', padding: 14, borderRadius: 8, marginVertical: 4, borderWidth: 1, borderColor: '#30363d' },
  deviceName: { color: '#f0f6fc', fontSize: 16, fontWeight: 'bold' },
  deviceId: { color: '#8b949e', fontSize: 12, marginTop: 2 },
  deviceRssi: { color: '#6e7681', fontSize: 12, marginTop: 2 },
  empty: { color: '#6e7681', textAlign: 'center', marginTop: 20 },
  disclaimer: { color: '#f85149', fontSize: 11, textAlign: 'center', marginTop: 10, marginBottom: 20 },
});