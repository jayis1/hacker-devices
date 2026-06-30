// src/screens/CaptureScreen.js — Live 802.15.4 frame capture view
// Author: jayis1
// License: MIT
import React, { useState } from 'react';
import { View, Text, FlatList, StyleSheet, TextInput, TouchableOpacity } from 'react-native';
import { useDevice } from '../context/DeviceContext';

const FRAME_TYPES = ['Beacon', 'Data', 'ACK', 'Cmd', 'RSVD', 'LLDN', 'Multi', 'Frag'];

function frameTypeLabel(frameHex) {
  if (!frameHex || frameHex.length < 4) return '?';
  const fc = parseInt(frameHex.slice(0, 4), 16);
  return FRAME_TYPES[fc & 0x07] || '?';
}

function panId(frameHex) {
  if (frameHex.length < 14) return '????';
  return frameHex.slice(6, 10);
}

export default function CaptureScreen() {
  const { frames, connected } = useDevice();
  const [filter, setFilter] = useState('');

  const filtered = filter
    ? frames.filter((f) => f.frame.toLowerCase().includes(filter.toLowerCase()))
    : frames;

  const renderItem = ({ item }) => (
    <View style={styles.row}>
      <Text style={styles.type}>{frameTypeLabel(item.frame)}</Text>
      <Text style={styles.ch}>CH{item.channel}</Text>
      <Text style={styles.rssi}>{item.rssi}dBm</Text>
      <Text style={styles.pan}>PAN:{panId(item.frame)}</Text>
      <Text style={styles.hex} numberOfLines={1}>{item.frame}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        <TextInput
          style={styles.filter}
          placeholder="Filter hex…"
          placeholderTextColor="#666"
          value={filter}
          onChangeText={setFilter}
        />
        <Text style={styles.count}>{filtered.length} frames</Text>
      </View>
      {!connected && <Text style={styles.warn}>Not connected to device</Text>}
      <FlatList
        data={filtered.slice().reverse()}
        keyExtractor={(item, idx) => String(idx)}
        renderItem={renderItem}
        style={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23' },
  toolbar: { flexDirection: 'row', padding: 8, alignItems: 'center' },
  filter: { flex: 1, backgroundColor: '#1a1a2e', color: '#ddd', borderRadius: 6, padding: 8, fontSize: 12 },
  count: { color: '#888', marginLeft: 8, fontSize: 12 },
  warn: { color: '#f44336', textAlign: 'center', padding: 8 },
  list: { flex: 1 },
  row: { flexDirection: 'row', padding: 8, borderBottomWidth: 0.5, borderBottomColor: '#1a1a2e', alignItems: 'center' },
  type: { color: '#e85d2c', fontSize: 10, fontWeight: 'bold', width: 60 },
  ch: { color: '#4caf50', fontSize: 10, width: 40 },
  rssi: { color: '#2196f3', fontSize: 10, width: 60 },
  pan: { color: '#ff9800', fontSize: 10, width: 70 },
  hex: { color: '#aaa', fontSize: 9, flex: 1, fontFamily: 'monospace' },
});