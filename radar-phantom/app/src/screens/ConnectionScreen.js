/**
 * ConnectionScreen.js — BLE scan & connect to RadarPhantom
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ConnectionScreen({ navigation }) {
  const { scan, connect } = useDevice();
  const [devices, setDevices] = useState([]);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    setScanning(true);
    const stop = scan((d) => {
      setDevices((prev) => {
        if (prev.find(x => x.id === d.id)) return prev;
        return [...prev, d];
      });
    }, (err) => {
      Alert.alert('BLE error', String(err));
      setScanning(false);
    });
    return () => { stop(); };
  }, []);

  const onConnect = async (d) => {
    try {
      await connect(d.id);
      navigation.navigate('Dashboard');
    } catch (e) {
      Alert.alert('Connect failed', String(e));
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>RadarPhantom</Text>
      <Text style={styles.subtitle}>mmWave FMCW Radar Phantom-Target Injector</Text>
      <Text style={styles.disclaimer}>
        ⚠ AUTHORIZED RESEARCH ONLY. Operate in shielded ranges only. Never near public roads.
      </Text>
      <Text style={styles.sectionLabel}>
        {scanning ? 'Scanning for RadarPhantom devices…' : `${devices.length} device(s) found`}
      </Text>
      <FlatList
        data={devices}
        keyExtractor={item => item.id}
        renderItem={({ item }) => (
          <TouchableOpacity style={styles.deviceRow} onPress={() => onConnect(item)}>
            <Text style={styles.deviceName}>{item.name || 'RadarPhantom'}</Text>
            <Text style={styles.deviceId}>{item.id}</Text>
            <Text style={styles.deviceRssi}>RSSI {item.rssi} dBm</Text>
          </TouchableOpacity>
        )}
        ListEmptyComponent={<Text style={styles.empty}>No devices yet…</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0f0f1a' },
  title: { color: '#00ff9f', fontSize: 26, fontWeight: 'bold', marginTop: 20 },
  subtitle: { color: '#888', fontSize: 14, marginTop: 4 },
  disclaimer: { color: '#ff5555', fontSize: 11, marginTop: 12, fontStyle: 'italic' },
  sectionLabel: { color: '#aaa', fontSize: 14, marginTop: 30, marginBottom: 10 },
  deviceRow: { padding: 16, backgroundColor: '#1a1a2e', borderRadius: 8, marginBottom: 8 },
  deviceName: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  deviceId: { color: '#666', fontSize: 12, marginTop: 4 },
  deviceRssi: { color: '#00ff9f', fontSize: 12, marginTop: 4 },
  empty: { color: '#555', fontStyle: 'italic', marginTop: 20 },
});