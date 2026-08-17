/**
 * CableMapScreen.js — aggregate TDR results into a cable inventory
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Each time the operator fires TDR on a new cable, the result is added
 * to a list. The list shows cable type, impedance, length, and a
 * user-editable location label. Useful for physical reconnaissance
 * during a facility walk-through.
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CABLE_TYPE_NAMES } from '../utils/cableTypes';

export default function CableMapScreen() {
  const { fireTDR, setTdrCallback, enterMode, MODE } = useDevice();
  const [cables, setCables] = useState([]);
  const [scanning, setScanning] = useState(false);

  const handleTdrData = useCallback((data, kind) => {
    if (kind === 'header') {
      const newCable = {
        id: Date.now(),
        type: data.type,
        impedance_mohm: data.impedance_mohm,
        length_mm: data.length_mm,
        shielded: data.shielded,
        live_conductor: data.live_conductor,
        label: '',
      };
      setCables((prev) => [...prev, newCable]);
    }
  }, []);

  const handleScan = async () => {
    setScanning(true);
    setTdrCallback(handleTdrData);
    await enterMode(MODE.TDR);
    await fireTDR();
    setTimeout(() => setScanning(false), 5000);
  };

  const handleClear = () => {
    setCables([]);
  };

  const renderItem = ({ item, index }) => {
    const zOhm = Math.round(item.impedance_mohm / 1000);
    const lenM = (item.length_mm / 1000).toFixed(2);
    return (
      <View style={styles.cableItem}>
        <Text style={styles.cableIndex}>#{index + 1}</Text>
        <Text style={[styles.cableType, { color: item.live_conductor ? '#f85149' : '#00d4aa' }]}>
          {CABLE_TYPE_NAMES[item.type] || 'Unknown'}
        </Text>
        <Text style={styles.cableDetail}>Z={zOhm}Ω  L={lenM}m</Text>
        <Text style={styles.cableDetail}>
          {item.shielded ? '🛡 shielded' : 'unshielded'} {item.live_conductor ? '⚡ LIVE' : ''}
        </Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Cable Map</Text>
      <Text style={styles.desc}>
        Walk the cable tray, clamp each cable, tap "Scan This Cable".
        Results aggregate into an inventory below.
      </Text>

      <View style={styles.toolbar}>
        <TouchableOpacity style={styles.scanButton} onPress={handleScan} disabled={scanning}>
          <Text style={styles.scanButtonText}>{scanning ? 'Scanning...' : '📍 Scan This Cable'}</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.clearButton} onPress={handleClear}>
          <Text style={styles.clearButtonText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.counter}>{cables.length} cables mapped</Text>

      <FlatList
        data={cables}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderItem}
        ListEmptyComponent={
          <Text style={styles.empty}>No cables mapped yet. Tap "Scan This Cable".</Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0d1117' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#a371f7', marginBottom: 4 },
  desc: { color: '#8b949e', fontSize: 12, marginBottom: 14 },
  toolbar: { flexDirection: 'row', gap: 10, marginBottom: 8 },
  scanButton: { backgroundColor: '#a371f7', padding: 12, borderRadius: 8, flex: 1, alignItems: 'center' },
  scanButtonText: { color: '#fff', fontWeight: 'bold' },
  clearButton: { backgroundColor: '#21262d', padding: 12, borderRadius: 8, borderWidth: 1, borderColor: '#30363d' },
  clearButtonText: { color: '#8b949e' },
  counter: { color: '#6e7681', fontSize: 12, marginBottom: 10 },
  cableItem: { backgroundColor: '#161b22', padding: 12, borderRadius: 8, marginBottom: 6, borderWidth: 1, borderColor: '#30363d' },
  cableIndex: { color: '#6e7681', fontSize: 11, marginBottom: 2 },
  cableType: { fontSize: 15, fontWeight: 'bold', marginBottom: 4 },
  cableDetail: { color: '#8b949e', fontSize: 12 },
  empty: { color: '#6e7681', textAlign: 'center', marginTop: 40, fontSize: 14 },
});