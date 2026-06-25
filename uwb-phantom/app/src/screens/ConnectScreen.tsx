/**
 * ConnectScreen.tsx — BLE scan + pair.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useCallback } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { Button, ActivityIndicator } from 'react-native-paper';
import { ble, useBle, BleDevice } from '../ble/UwbPhantomBle';

export default function ConnectScreen(): JSX.Element {
  const { device, setDevice } = useBle();
  const [scanning, setScanning] = useState(false);
  const [results, setResults] = useState<BleDevice[]>([]);
  const [error, setError] = useState<string>('');

  const scan = useCallback(async () => {
    setScanning(true); setError(''); setResults([]);
    try {
      const found = await ble.scan(5000);
      setResults(found);
    } catch (e: any) {
      setError(e.message ?? String(e));
    } finally {
      setScanning(false);
    }
  }, []);

  const connect = useCallback(async (d: BleDevice) => {
    try {
      await ble.connect(d.id);
      setDevice(d);
    } catch (e: any) {
      setError(e.message ?? String(e));
    }
  }, [setDevice]);

  return (
    <View style={styles.root}>
      <Text style={styles.title}>Connect to UWB-PHANTOM</Text>
      <Button mode="contained" onPress={scan} loading={scanning}>
        {scanning ? 'Scanning…' : 'Scan for devices'}
      </Button>
      {error ? <Text style={styles.error}>{error}</Text> : null}
      {device && <Text style={styles.connected}>✓ {device.name}</Text>}
      <FlatList
        data={results}
        keyExtractor={i => i.id}
        renderItem={({ item }) => (
          <TouchableOpacity style={styles.row} onPress={() => connect(item)}>
            <Text style={styles.name}>{item.name}</Text>
            <Text style={styles.rssi}>{item.rssi} dBm</Text>
          </TouchableOpacity>
        )}
      />
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  error:   { color: '#ff5252', fontSize: 12, marginTop: 8 },
  connected: { color: '#69f0ae', fontSize: 14, marginTop: 8 },
  row:     { flexDirection: 'row', justifyContent: 'space-between',
             padding: 14, borderBottomWidth: 1, borderBottomColor: '#1c242c' },
  name:    { color: '#e0e0e0', fontSize: 14 },
  rssi:    { color: '#90a4ae', fontSize: 12 },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 12 },
});