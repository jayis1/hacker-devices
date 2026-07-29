/**
 * screens/DmaScreen.js — DMA Bridge control + Hot-Plug fault injection
 *
 * Author: jayis1
 * License: MIT
 *
 * Provides the two-stage DMA arm/go sequence and the hot-plug fault
 * trigger.  DMA operations require explicit confirmation per the
 * firmware safety interlock.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';

export default function DmaScreen({ ble }) {
  const [hostPhys, setHostPhys] = useState('0x100000');
  const [len, setLen] = useState('512');
  const [dir, setDir] = useState('0');  // 0 = read FROM host, 1 = write TO host
  const [armToken, setArmToken] = useState(null);
  const [result, setResult] = useState(null);
  const [busy, setBusy] = useState(false);

  const handleArm = async () => {
    setBusy(true);
    try {
      const phys = BigInt(hostPhys) >>> 0n || parseInt(hostPhys, 16);
      const l = parseInt(len, 10);
      const token = await ble.dmaArm(Number(phys), l, parseInt(dir, 10));
      setArmToken(token);
      Alert.alert('DMA Armed', `Token: 0x${token.toString(16).toUpperCase()}\n\nReview and press GO to execute.`);
    } catch (e) { Alert.alert('Arm Error', e.message); }
    setBusy(false);
  };

  const handleGo = async () => {
    if (armToken === null) { Alert.alert('Not armed', 'Press ARM first'); return; }
    Alert.alert(
      '⚠️ Confirm DMA',
      `About to ${dir === '0' ? 'read FROM' : 'write TO'} host phys ${hostPhys} (${len} bytes).\n\nThis issues a synthetic NVMe command with PRP1 = target address. Proceed?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'GO', style: 'destructive', onPress: async () => {
          setBusy(true);
          try {
            const r = await ble.dmaGo(armToken);
            setResult(r.payload);
            setArmToken(null);
          } catch (e) { Alert.alert('DMA Error', e.message); }
          setBusy(false);
        }}
      ]
    );
  };

  const handleHotplug = (event) => {
    Alert.alert(
      event === 0 ? 'Surprise Removal' : 'Replug',
      `This will ${event === 0 ? 'remove' : 're-plug'} the SSD from the host PCIe link.\n\nOn a live OS this triggers the NVMe hot-plug handler and may crash the storage stack. Proceed?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: event === 0 ? 'REMOVE' : 'REPLUG', style: 'destructive', onPress: async () => {
          try { await ble.hotplug(event); } catch (e) { Alert.alert('Error', e.message); }
        }}
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>DMA Bridge</Text>
      <Text style={styles.warning}>⚠️ DMA Bridge issues synthetic NVMe commands with PRP1 = target host physical address.{'\n'}This is a memory-access primitive. Use only on systems you own.</Text>

      <Text style={styles.label}>Host Physical Address (hex)</Text>
      <TextInput style={styles.input} value={hostPhys} onChangeText={setHostPhys} placeholder="0x100000"/>

      <Text style={styles.label}>Length (bytes, multiple of 512)</Text>
      <TextInput style={styles.input} value={len} onChangeText={setLen} keyboardType="numeric"/>

      <Text style={styles.label}>Direction</Text>
      <View style={styles.dirRow}>
        <TouchableOpacity style={[styles.dirBtn, dir === '0' && styles.dirActive]} onPress={() => setDir('0')}>
          <Text style={styles.dirText}>Read FROM host</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.dirBtn, dir === '1' && styles.dirActive]} onPress={() => setDir('1')}>
          <Text style={styles.dirText}>Write TO host</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.btnArm} onPress={handleArm} disabled={busy}>
          <Text style={styles.btnText}>1. ARM</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.btnGo, armToken === null && styles.btnDisabled]} onPress={handleGo} disabled={busy || armToken === null}>
          <Text style={styles.btnText}>2. GO</Text>
        </TouchableOpacity>
      </View>

      {armToken !== null && (
        <Text style={styles.token}>Armed. Token: 0x{armToken.toString(16).toUpperCase()}</Text>
      )}
      {result && (
        <View style={styles.resultBox}>
          <Text style={styles.resultTitle}>Result</Text>
          <Text style={styles.resultHex}>{Array.from(result).map(b => b.toString(16).padStart(2, '0')).join(' ')}</Text>
        </View>
      )}

      <Text style={styles.sectionTitle}>Hot-Plug Fault Injection</Text>
      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.btnRed} onPress={() => handleHotplug(0)}>
          <Text style={styles.btnText}>Surprise Remove</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnGreen} onPress={() => handleHotplug(1)}>
          <Text style={styles.btnText}>Replug</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.footer}>Two-stage DMA interlock — jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 5 },
  warning: { color: '#FF6600', fontSize: 11, marginBottom: 15 },
  label: { color: '#aaa', fontSize: 14, marginTop: 10, marginBottom: 5 },
  input: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, padding: 10, fontSize: 14, marginBottom: 5, fontFamily: 'monospace' },
  dirRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 10 },
  dirBtn: { backgroundColor: '#16213e', padding: 10, borderRadius: 6, width: '48%', alignItems: 'center' },
  dirActive: { backgroundColor: '#0f3460' },
  dirText: { color: '#eee', fontSize: 13 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 15 },
  btnArm: { backgroundColor: '#cca300', padding: 15, borderRadius: 8, width: '48%', alignItems: 'center' },
  btnGo: { backgroundColor: '#cc3300', padding: 15, borderRadius: 8, width: '48%', alignItems: 'center' },
  btnDisabled: { backgroundColor: '#333' },
  btnRed: { backgroundColor: '#cc3300', padding: 12, borderRadius: 6, width: '48%', alignItems: 'center' },
  btnGreen: { backgroundColor: '#00AA00', padding: 12, borderRadius: 6, width: '48%', alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 15, fontWeight: 'bold' },
  token: { color: '#cca300', fontSize: 13, marginTop: 10, fontFamily: 'monospace' },
  resultBox: { backgroundColor: '#0d1117', borderRadius: 6, padding: 10, marginTop: 10 },
  resultTitle: { color: '#00AA00', fontSize: 14, fontWeight: 'bold', marginBottom: 5 },
  resultHex: { color: '#aaa', fontSize: 11, fontFamily: 'monospace' },
  sectionTitle: { fontSize: 18, color: '#888', marginTop: 25, marginBottom: 10 },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 15 },
});