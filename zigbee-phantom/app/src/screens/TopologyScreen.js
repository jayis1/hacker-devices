// src/screens/TopologyScreen.js — Discovered Zigbee mesh device map
// Author: jayis1
// License: MIT
import React from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';
import { useDevice } from '../context/DeviceContext';

export default function TopologyScreen() {
  const { topology } = useDevice();
  const devices = Object.values(topology).sort((a, b) => b.lastSeen - a.lastSeen);

  const renderItem = ({ item }) => {
    const ageS = Math.floor((Date.now() - item.lastSeen) / 1000);
    return (
      <View style={styles.row}>
        <Text style={styles.eui}>{item.eui}</Text>
        <Text style={styles.pan}>PAN:{item.pan || '????'}</Text>
        <Text style={styles.ch}>CH{item.channel}</Text>
        <Text style={styles.rssi}>{item.rssi}dBm</Text>
        <Text style={styles.age}>{ageS}s ago</Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.header}>Discovered Devices ({devices.length})</Text>
      {devices.length === 0 && (
        <Text style={styles.empty}>No devices discovered yet. Start sniffing to map the mesh.</Text>
      )}
      <FlatList
        data={devices}
        keyExtractor={(item) => item.eui}
        renderItem={renderItem}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 12 },
  header: { color: '#e85d2c', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  empty: { color: '#888', textAlign: 'center', marginTop: 40 },
  row: { flexDirection: 'row', padding: 10, borderBottomWidth: 0.5, borderBottomColor: '#1a1a2e', alignItems: 'center' },
  eui: { color: '#ddd', fontSize: 11, fontFamily: 'monospace', flex: 1 },
  pan: { color: '#ff9800', fontSize: 10, width: 70 },
  ch: { color: '#4caf50', fontSize: 10, width: 40 },
  rssi: { color: '#2196f3', fontSize: 10, width: 55 },
  age: { color: '#888', fontSize: 10, width: 60 },
});