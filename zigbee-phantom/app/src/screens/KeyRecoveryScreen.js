// src/screens/KeyRecoveryScreen.js — Transport-Key capture & install-code brute force
// Author: jayis1
// License: MIT
import React, { useState } from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity, TextInput, Alert } from 'react-native';
import { useDevice } from '../context/DeviceContext';

export default function KeyRecoveryScreen() {
  const { keyFrames, commands } = useDevice();
  const [selectedKey, setSelectedKey] = useState(null);
  const [icLen, setIcLen] = useState('4');
  const [bruting, setBruting] = useState(false);

  function startBrute() {
    if (!selectedKey) { Alert.alert('Select a captured key first'); return; }
    const len = parseInt(icLen, 10);
    if (![4, 8, 12, 16].includes(len)) { Alert.alert('Install-code length must be 4/8/12/16'); return; }
    setBruting(true);
    commands.bruteKey(selectedKey.keyBlob.slice(0, 32), len)
      .then(() => { setBruting(false); Alert.alert('Brute-force submitted', 'Watch for KEY_CAPTURED event with result.'); })
      .catch((e) => { setBruting(false); Alert.alert('Error', String(e)); });
  }

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={[styles.krow, selectedKey === item && styles.krowSel]}
      onPress={() => setSelectedKey(item)}
    >
      <Text style={styles.ktype}>{item.keyType}</Text>
      <Text style={styles.kblob} numberOfLines={1}>{item.keyBlob}</Text>
      <Text style={styles.ktime}>{new Date(item.time).toLocaleTimeString()}</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.header}>Captured Transport-Key Frames</Text>
      {keyFrames.length === 0 && (
        <Text style={styles.empty}>
          No Transport-Key frames captured yet. Join a device to the mesh (or
          power-cycle a battery sensor) to trigger key transport.
        </Text>
      )}
      <FlatList
        data={keyFrames}
        keyExtractor={(item, idx) => String(idx)}
        renderItem={renderItem}
        style={{ maxHeight: 200 }}
      />
      {selectedKey && (
        <View style={styles.detailCard}>
          <Text style={styles.dtitle}>Selected Key Blob</Text>
          <Text style={styles.dhex}>{selectedKey.keyBlob}</Text>
          <Text style={styles.dhint}>Encrypted network key (16 bytes = 32 hex chars)</Text>

          <Text style={styles.dtitle}>Install-Code Brute Force</Text>
          <View style={styles.row}>
            <Text style={styles.label}>IC length:</Text>
            <TextInput style={styles.input} value={icLen} onChangeText={setIcLen} keyboardType="numeric" />
            <TouchableOpacity style={styles.bruteBtn} onPress={startBrute} disabled={bruting}>
              <Text style={styles.bruteBtnText}>{bruting ? 'Running…' : 'Start Brute'}</Text>
            </TouchableOpacity>
          </View>
          <Text style={styles.dhint}>
            4-byte codes: seconds. 8-byte codes: hours. The device uses hardware AES.
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 12 },
  header: { color: '#e85d2c', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  empty: { color: '#888', textAlign: 'center', marginTop: 30 },
  krow: { flexDirection: 'row', padding: 10, borderBottomWidth: 0.5, borderBottomColor: '#1a1a2e', alignItems: 'center' },
  krowSel: { backgroundColor: '#2a2a4e' },
  ktype: { color: '#e85d2c', fontSize: 10, width: 80 },
  kblob: { color: '#ddd', fontSize: 10, flex: 1, fontFamily: 'monospace' },
  ktime: { color: '#888', fontSize: 10, width: 70 },
  detailCard: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginTop: 12 },
  dtitle: { color: '#e85d2c', fontSize: 12, fontWeight: 'bold', marginTop: 8 },
  dhex: { color: '#ddd', fontSize: 11, fontFamily: 'monospace', marginTop: 4 },
  dhint: { color: '#888', fontSize: 10, marginTop: 4 },
  row: { flexDirection: 'row', alignItems: 'center', marginTop: 8 },
  label: { color: '#ddd', fontSize: 12 },
  input: { backgroundColor: '#0f0f23', color: '#ddd', width: 50, marginHorizontal: 8, borderRadius: 4, padding: 4, fontSize: 12 },
  bruteBtn: { backgroundColor: '#e85d2c', padding: 8, borderRadius: 6, marginLeft: 'auto' },
  bruteBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
});