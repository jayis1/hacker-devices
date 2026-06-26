/**
 * PlaintextManagerScreen.js — generate/load known-plaintext lists
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * CPA requires known plaintexts. This screen lets the operator generate a
 * random plaintext list, paste a custom list, or select the feed mode
 * (RANDOM generated on-device / DRIVE via UART / LISTEN passively).
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

function randomHex(len) {
  const chars = '0123456789abcdef';
  let s = '';
  for (let i = 0; i < len; i++) s += chars[Math.floor(Math.random() * 16)];
  return s;
}

export default function PlaintextManagerScreen() {
  const { sendCommand } = useDevice();
  const [ptList, setPtList] = useState('');
  const [count, setCount] = useState('16');

  const generateRandom = useCallback(() => {
    const n = Math.max(1, Math.min(1000, parseInt(count, 10) || 16));
    const lines = [];
    for (let i = 0; i < n; i++) lines.push(randomHex(32)); // 16 bytes = 32 hex chars
    setPtList(lines.join('\n'));
  }, [count]);

  const clearList = useCallback(() => setPtList(''), []);

  const pushToDevice = useCallback(async () => {
    // In a full deployment, plaintexts would be bulk-uploaded via a dedicated
    // BLE characteristic. Here we just confirm the mode is set.
    const resp = await sendCommand('SET PTMODE DRIVE', 2000);
    Alert.alert('Plaintext mode', resp || 'No response');
  }, [sendCommand]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Plaintext Manager</Text>
      <Text style={styles.hint}>CPA needs known plaintexts. Generate a list or supply your own.</Text>

      <View style={styles.row}>
        <Text style={styles.label}>Count:</Text>
        <TextInput style={styles.input} value={count} onChangeText={setCount} keyboardType="numeric" />
        <TouchableOpacity style={styles.genBtn} onPress={generateRandom}>
          <Text style={styles.genBtnText}>🎲 Generate</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.sectionLabel}>Plaintext List (16 bytes each, hex)</Text>
      <TextInput
        style={styles.textArea}
        value={ptList}
        onChangeText={setPtList}
        multiline
        placeholder="00112233445566778899aabbccddeeff"
        placeholderTextColor="#6e7681"
        textAlignVertical="top"
      />

      <View style={styles.btnRow}>
        <TouchableOpacity style={styles.btn} onPress={clearList}>
          <Text style={styles.btnText}>Clear</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btn} onPress={() => setPtList(ptList.split('\n').slice(0, 16).join('\n'))}>
          <Text style={styles.btnText}>First 16</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnPrimary} onPress={pushToDevice}>
          <Text style={styles.btnText}>→ Set DRIVE mode</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.infoBox}>
        <Text style={styles.infoTitle}>Feed Modes</Text>
        <Text style={styles.infoItem}>• RANDOM — device generates plaintexts internally (LFSR). Simplest.</Text>
        <Text style={styles.infoItem}>• DRIVE — device pushes each plaintext to target over UART before each trace.</Text>
        <Text style={styles.infoItem}>• LISTEN — target is driven externally; device just captures + records plaintext.</Text>
      </View>

      <Text style={styles.footer}>Author: jayis1 — authorized use only</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35', marginBottom: 4 },
  hint: { color: '#8b949e', fontSize: 12, marginBottom: 16 },
  row: { flexDirection: 'row', alignItems: 'center', gap: 8, marginBottom: 16 },
  label: { color: '#8b949e', fontSize: 14 },
  input: { backgroundColor: '#161b22', color: '#e6edf3', borderWidth: 1, borderColor: '#30363d', borderRadius: 6, padding: 8, width: 80, fontFamily: 'monospace' },
  genBtn: { backgroundColor: '#ff6b35', padding: 10, borderRadius: 6, marginLeft: 8 },
  genBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
  sectionLabel: { color: '#e6edf3', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  textArea: { backgroundColor: '#161b22', color: '#58a6ff', borderWidth: 1, borderColor: '#30363d', borderRadius: 8, padding: 10, minHeight: 200, fontFamily: 'monospace', fontSize: 12, marginBottom: 16 },
  btnRow: { flexDirection: 'row', gap: 8, marginBottom: 16 },
  btn: { flex: 1, backgroundColor: '#21262d', padding: 12, borderRadius: 8, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  btnPrimary: { flex: 2, backgroundColor: '#238636', padding: 12, borderRadius: 8, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  infoBox: { backgroundColor: '#161b22', borderRadius: 8, padding: 12, borderWidth: 1, borderColor: '#30363d' },
  infoTitle: { color: '#58a6ff', fontSize: 13, fontWeight: 'bold', marginBottom: 6 },
  infoItem: { color: '#8b949e', fontSize: 12, lineHeight: 18 },
  footer: { color: '#6e7681', fontSize: 11, marginTop: 16, textAlign: 'center' },
});