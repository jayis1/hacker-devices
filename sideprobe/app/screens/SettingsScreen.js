/**
 * SettingsScreen.js — gain, shunt, model, threshold, firmware info
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function SettingsScreen() {
  const { sendCommand, disconnect } = useDevice();
  const [margin, setMargin] = useState('3.0');
  const [shunt, setShunt] = useState('10');
  const [leakIdx, setLeakIdx] = useState('614');
  const [info, setInfo] = useState('');

  const sendMargin = useCallback(async () => {
    const resp = await sendCommand(`SET MARGIN ${margin}`, 2000);
    Alert.alert('Margin', resp || 'No response');
  }, [sendCommand, margin]);

  const sendHelp = useCallback(async () => {
    const resp = await sendCommand('HELP', 2000);
    setInfo(resp || 'No response');
  }, [sendCommand]);

  const sendStatus = useCallback(async () => {
    const resp = await sendCommand('STATUS', 2000);
    setInfo(resp || 'No response');
  }, [sendCommand]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Convergence Margin</Text>
        <Text style={styles.cardHint}>Best/2nd-best correlation ratio for early-stop (default 3.0×)</Text>
        <View style={styles.row}>
          <TextInput style={styles.input} value={margin} onChangeText={setMargin} keyboardType="numeric" />
          <TouchableOpacity style={styles.btn} onPress={sendMargin}>
            <Text style={styles.btnText}>Set</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Shunt Resistance (Ω)</Text>
        <Text style={styles.cardHint}>Used for power-modality gain calculation</Text>
        <View style={styles.row}>
          <TextInput style={styles.input} value={shunt} onChangeText={setShunt} keyboardType="numeric" />
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Leak Sample Index</Text>
        <Text style={styles.cardHint}>Sample point whose correlation is evaluated (0–2047)</Text>
        <View style={styles.row}>
          <TextInput style={styles.input} value={leakIdx} onChangeText={setLeakIdx} keyboardType="numeric" />
        </View>
      </View>

      <View style={styles.btnRow}>
        <TouchableOpacity style={styles.btnPrimary} onPress={sendStatus}>
          <Text style={styles.btnText}>Query STATUS</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnPrimary} onPress={sendHelp}>
          <Text style={styles.btnText}>HELP</Text>
        </TouchableOpacity>
      </View>

      {info ? (
        <View style={styles.infoBox}>
          <Text style={styles.infoText}>{info}</Text>
        </View>
      ) : null}

      <View style={styles.aboutBox}>
        <Text style={styles.aboutTitle}>SideProbe v1.0</Text>
        <Text style={styles.aboutText}>Portable Power & EM Side-Channel Cryptanalysis Platform</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Hardware: CERN-OHL-S v2 | Firmware/App: GPL-2.0 / MIT</Text>
        <Text style={styles.disclaimer}>
          ⚠️ LEGAL: For authorized security research only. Recovering crypto
          keys from devices you do not own or are not authorized to assess
          may violate computer-fraud, trade-secret, and telecom-interception
          laws. Always obtain written authorization before deployment.
        </Text>
      </View>

      <TouchableOpacity style={styles.disconnectBtn} onPress={disconnect}>
        <Text style={styles.btnText}>Disconnect</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35', marginBottom: 16 },
  card: { backgroundColor: '#161b22', borderRadius: 10, padding: 14, marginBottom: 12, borderWidth: 1, borderColor: '#30363d' },
  cardTitle: { color: '#e6edf3', fontSize: 15, fontWeight: 'bold', marginBottom: 4 },
  cardHint: { color: '#8b949e', fontSize: 12, marginBottom: 8 },
  row: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  input: { backgroundColor: '#0d1117', color: '#58a6ff', borderWidth: 1, borderColor: '#30363d', borderRadius: 6, padding: 8, width: 100, fontFamily: 'monospace', flex: 1 },
  btn: { backgroundColor: '#21262d', padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#30363d' },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  btnRow: { flexDirection: 'row', gap: 12, marginBottom: 16 },
  btnPrimary: { flex: 1, backgroundColor: '#21262d', padding: 12, borderRadius: 8, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  infoBox: { backgroundColor: '#161b22', borderRadius: 8, padding: 12, marginBottom: 16, borderWidth: 1, borderColor: '#30363d' },
  infoText: { color: '#58a6ff', fontFamily: 'monospace', fontSize: 11 },
  aboutBox: { backgroundColor: '#161b22', borderRadius: 10, padding: 16, marginBottom: 16, borderWidth: 1, borderColor: '#30363d' },
  aboutTitle: { color: '#ff6b35', fontSize: 16, fontWeight: 'bold', marginBottom: 6 },
  aboutText: { color: '#8b949e', fontSize: 12, marginBottom: 2 },
  disclaimer: { color: '#f0883e', fontSize: 11, marginTop: 10, lineHeight: 16 },
  disconnectBtn: { backgroundColor: '#6e2c2c', padding: 14, borderRadius: 8, alignItems: 'center' },
});