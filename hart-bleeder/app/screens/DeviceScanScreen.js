/**
 * hart-bleeder — DeviceScanScreen.js
 * HART bus enumeration: triggers a Command 0 sweep across all 64
 * poll addresses and displays discovered field devices with
 * manufacturer, device type, and unique ID.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  SafeAreaView, StatusBar, ActivityIndicator, Alert,
} from 'react-native';

export default function DeviceScanScreen({ bleManager, connected, devices }) {
  const [scanning, setScanning] = useState(false);
  const [selected, setSelected] = useState(null);

  const handleScan = async () => {
    if (!connected) { Alert.alert('Not connected'); return; }
    setScanning(true);
    try {
      await bleManager.scanBus();
    } catch (e) {
      Alert.alert('Scan Error', e.message);
    }
    setTimeout(() => setScanning(false), 5000);
  };

  const renderItem = ({ item, index }) => {
    const mfr = item.manufacturer || 0;
    const devType = item.dev_type || 0;
    const id = item.long_addr || [];
    const idHex = Array.from(id).map((b) => b.toString(16).padStart(2, '0')).join('');
    return (
      <TouchableOpacity
        style={[styles.deviceCard, selected === index && styles.selectedCard]}
        onPress={() => setSelected(index)}
      >
        <View style={styles.deviceHeader}>
          <Text style={styles.deviceAddr}>Addr {item.poll_addr || 0}</Text>
          <Text style={styles.deviceMfr}>Mfr 0x{mfr.toString(16).padStart(2, '0')}</Text>
          <Text style={styles.deviceType}>Type 0x{devType.toString(16).padStart(2, '0')}</Text>
        </View>
        <Text style={styles.deviceId}>Unique ID: {idHex}</Text>
        {item.tag && (
          <Text style={styles.deviceTag}>Tag: {String.fromCharCode(...(item.tag || []))}</Text>
        )}
      </TouchableOpacity>
    );
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <View style={styles.toolbar}>
        <Text style={styles.toolbarTitle}>HART Bus Enumeration</Text>
        <TouchableOpacity style={styles.scanBtn} onPress={handleScan} disabled={scanning}>
          {scanning ? <ActivityIndicator color="#fff" /> :
            <Text style={styles.scanBtnText}>Scan (Cmd 0)</Text>}
        </TouchableOpacity>
      </View>

      <View style={styles.infoBox}>
        <Text style={styles.infoText}>
          Sweeps poll addresses 0–63 issuing HART Command 0 (Read Unique ID).
          Each responding device's manufacturer code, device type, and
          38-bit unique identifier are recorded. Multi-drop loops can host
          up to 64 devices; point-to-point loops have a single primary device.
        </Text>
      </View>

      <FlatList
        data={devices}
        keyExtractor={(item, i) => i.toString()}
        renderItem={renderItem}
        ListEmptyComponent={
          <Text style={styles.empty}>No devices discovered. Tap Scan to enumerate the bus.</Text>
        }
        contentContainerStyle={{ paddingBottom: 20 }}
      />

      <Text style={styles.disclaimer}>
        ⚠ Authorized research only — enumerating process control loops you do not
        own may violate industrial facility security and safety regulations.
      </Text>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  toolbar: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  toolbarTitle: { color: '#e94560', fontSize: 18, fontWeight: 'bold' },
  scanBtn: { backgroundColor: '#e94560', padding: 10, borderRadius: 8 },
  scanBtnText: { color: '#fff', fontWeight: 'bold' },
  infoBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 10, marginBottom: 12 },
  infoText: { color: '#aaa', fontSize: 11 },
  deviceCard: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginBottom: 10, borderWidth: 1, borderColor: '#2d2d44' },
  selectedCard: { borderColor: '#e94560', borderWidth: 2 },
  deviceHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  deviceAddr: { color: '#4a90d9', fontWeight: 'bold', fontSize: 14 },
  deviceMfr: { color: '#10b981', fontSize: 12 },
  deviceType: { color: '#f59e0b', fontSize: 12 },
  deviceId: { color: '#888', fontSize: 11, fontFamily: 'monospace' },
  deviceTag: { color: '#aaa', fontSize: 12, marginTop: 4 },
  empty: { color: '#555', textAlign: 'center', marginTop: 40, fontSize: 13 },
  disclaimer: { color: '#f59e0b', fontSize: 9, textAlign: 'center', marginTop: 8 },
});