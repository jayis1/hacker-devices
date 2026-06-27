/**
 * Tactile-Phantom — Companion App
 * screens/ConnectScreen.tsx — BLE device scanning and connection
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, TouchableOpacity, FlatList, StyleSheet,
  ActivityIndicator, Alert,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { bleManager } from '../src/ble';
import { useStore } from '../src/store';

interface ScannedDevice {
  id: string;
  name: string;
  rssi: number;
}

export default function ConnectScreen() {
  const navigation = useNavigation();
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState<ScannedDevice[]>([]);
  const [connecting, setConnecting] = useState(false);
  const setConnected = useStore((s) => s.setConnected);

  const startScan = useCallback(async () => {
    setScanning(true);
    setDevices([]);
    try {
      const permsOk = await bleManager.requestPermissions();
      if (!permsOk) {
        Alert.alert('Permission Denied', 'BLE permissions are required');
        setScanning(false);
        return;
      }
      const found = await bleManager.scanForDevices(10000);
      setDevices(found.map((d) => ({
        id: d.id,
        name: d.name || 'Tactile-Phantom',
        rssi: d.rssi || 0,
      })));
    } catch (error) {
      Alert.alert('Scan Error', String(error));
    }
    setScanning(false);
  }, []);

  const connectToDevice = useCallback(async (deviceId: string, name: string) => {
    setConnecting(true);
    try {
      const ok = await bleManager.connect(deviceId);
      if (ok) {
        setConnected(true, name);
        // Register callbacks for event/status streaming
        bleManager.onEvent((event) => {
          useStore.getState().addEvent(event);
        });
        bleManager.onStatus((status) => {
          useStore.getState().setStatus(status);
        });
        navigation.navigate('LiveCapture');
      } else {
        Alert.alert('Connection Failed', 'Could not connect to device');
      }
    } catch (error) {
      Alert.alert('Connection Error', String(error));
    }
    setConnecting(false);
  }, [navigation, setConnected]);

  const renderItem = ({ item }: { item: ScannedDevice }) => (
    <TouchableOpacity
      style={styles.deviceItem}
      onPress={() => connectToDevice(item.id, item.name)}
      disabled={connecting}
    >
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item.name}</Text>
        <Text style={styles.deviceId}>{item.id}</Text>
      </View>
      <Text style={styles.rssi}>{item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Tactile-Phantom</Text>
        <Text style={styles.subtitle}>Touch-Controller Bus MITM Implant</Text>
        <Text style={styles.author}>by jayis1 · GPL-2.0</Text>
      </View>

      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠️ For authorized security research only. Obtain written consent
          before deploying on any device you do not own.
        </Text>
      </View>

      <TouchableOpacity style={styles.scanButton} onPress={startScan} disabled={scanning}>
        {scanning ? (
          <View style={styles.scanRow}>
            <ActivityIndicator color="#e0e0e0" />
            <Text style={styles.scanButtonText}>Scanning...</Text>
          </View>
        ) : (
          <Text style={styles.scanButtonText}>Scan for Devices</Text>
        )}
      </TouchableOpacity>

      {connecting && (
        <View style={styles.connectingRow}>
          <ActivityIndicator color="#00ff88" />
          <Text style={styles.connectingText}>Connecting...</Text>
        </View>
      )}

      <FlatList
        data={devices}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
        style={styles.deviceList}
        ListEmptyComponent={
          !scanning && devices.length === 0 ? (
            <Text style={styles.emptyText}>No devices found. Tap Scan.</Text>
          ) : null
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0f0f1a' },
  header: { alignItems: 'center', marginBottom: 20, marginTop: 20 },
  title: { fontSize: 28, fontWeight: 'bold', color: '#e0e0e0' },
  subtitle: { fontSize: 14, color: '#888', marginTop: 5 },
  author: { fontSize: 12, color: '#666', marginTop: 3 },
  disclaimer: {
    backgroundColor: '#3a1a1a', borderRadius: 8, padding: 10, marginBottom: 20,
    borderWidth: 1, borderColor: '#cc3333',
  },
  disclaimerText: { color: '#cc8888', fontSize: 11 },
  scanButton: {
    backgroundColor: '#16213e', borderRadius: 10, padding: 15,
    alignItems: 'center', borderWidth: 1, borderColor: '#0f3460',
  },
  scanRow: { flexDirection: 'row', alignItems: 'center', gap: 10 },
  scanButtonText: { color: '#e0e0e0', fontSize: 16, fontWeight: 'bold' },
  connectingRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', gap: 10, marginTop: 10 },
  connectingText: { color: '#00ff88', fontSize: 14 },
  deviceList: { flex: 1, marginTop: 15 },
  deviceItem: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
    backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 8,
    borderWidth: 1, borderColor: '#0f3460',
  },
  deviceInfo: { flex: 1 },
  deviceName: { color: '#e0e0e0', fontSize: 16, fontWeight: 'bold' },
  deviceId: { color: '#666', fontSize: 11, marginTop: 2 },
  rssi: { color: '#00ff88', fontSize: 13 },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 20 },
});