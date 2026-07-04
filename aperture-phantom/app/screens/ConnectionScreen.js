/**
 * AperturePhantom — ConnectionScreen
 *
 * Scans for BLE devices advertising the APERTURE name, lets the operator
 * pair and authenticate against the device's DS28E07 secure element.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useEffect, useState } from 'react';
import { View, Text, Button, FlatList, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ConnectionScreen({ navigation }) {
  const { connected, scanResults, startScan, stopScan, connect, log } = useDevice();
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    startScan();
    return () => stopScan();
  }, []);

  useEffect(() => {
    if (connected) navigation.replace('Dashboard');
  }, [connected]);

  const onConnect = async (id) => {
    setBusy(true);
    stopScan();
    try { await connect(id); }
    catch (e) { Alert.alert('Connect failed', e.message); }
    finally { setBusy(false); }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>AperturePhantom</Text>
      <Text style={styles.h2}>MIPI CSI-2 Camera Interposer</Text>
      <Text style={styles.author}>author: jayis1 · GPL-2.0</Text>
      <Text style={styles.warn}>
        ⚠ Authorized research only. Capturing imagery of persons may require
        consent under applicable law.
      </Text>

      <View style={styles.row}>
        <Button title="Rescan" onPress={startScan} disabled={busy} />
        <Button title="Stop"   onPress={stopScan} disabled={busy} />
      </View>

      <Text style={styles.section}>Found devices ({scanResults.length})</Text>
      <FlatList
        data={scanResults}
        keyExtractor={(d) => d.id}
        renderItem={({ item }) => (
          <View style={styles.devRow}>
            <Text style={styles.devName}>{item.name || 'unknown'}</Text>
            <Text style={styles.devId}>{item.id}</Text>
            <Button title="Connect" onPress={() => onConnect(item.id)} />
          </View>
        )}
        ListEmptyComponent={<Text style={styles.empty}>No devices yet…</Text>}
      />

      <Text style={styles.section}>Activity</Text>
      <FlatList
        data={log.slice(-8)}
        keyExtractor={(l, i) => String(i)}
        renderItem={({ item }) => <Text style={styles.log}>{item}</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418' },
  h1: { color: '#d0e0f0', fontSize: 24, fontWeight: '700' },
  h2: { color: '#8aa0b8', fontSize: 14, marginTop: 4 },
  author: { color: '#5a7088', fontSize: 11, marginTop: 2, marginBottom: 8 },
  warn: { color: '#e0a040', fontSize: 11, marginBottom: 12, fontStyle: 'italic' },
  row: { flexDirection: 'row', gap: 8, marginBottom: 12 },
  section: { color: '#a0b0c0', fontSize: 13, fontWeight: '600', marginTop: 12 },
  devRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
            paddingVertical: 8, borderBottomColor: '#202830', borderBottomWidth: 1 },
  devName: { color: '#d0e0f0', fontSize: 14 },
  devId: { color: '#708090', fontSize: 10, flex: 1, marginLeft: 8 },
  empty: { color: '#607080', fontSize: 12, paddingVertical: 8 },
  log: { color: '#7a9098', fontSize: 10, fontFamily: 'monospace' },
});