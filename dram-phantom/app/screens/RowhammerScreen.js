/**
 * RowhammerScreen.js — Rowhammer injection control panel
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Lets the operator select an aggressor row, hammer count, and pattern
 * (single/double/many-sided or TRR-defeating), arm the device with the
 * two-key token, run the hammer, and view detected bit-flips in a heatmap.
 *
 * SAFETY: arming requires reading the token from the device OLED and entering
 * it here. The device firmware enforces the same check.
 */

import React, { useState, useCallback } from 'react';
import { View, Text, StyleSheet, Button, TextInput, TouchableOpacity, Alert, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const PATTERNS = [
  { id: 0, name: 'Single-sided (R → R+1)' },
  { id: 1, name: 'Double-sided (R-1,R+1 → R)' },
  { id: 2, name: 'Many-sided (R±2,R±1 → R)' },
  { id: 3, name: 'TRR-defeating (interleaved REF)' },
];

export default function RowhammerScreen() {
  const { connected, status, setMode, arm, disarm, hammerArm, hammerRun } = useDevice();
  const [aggressor, setAggressor] = useState('0');
  const [count, setCount] = useState('100000');
  const [pattern, setPattern] = useState(0);
  const [token, setToken] = useState('');
  const [armed, setArmed] = useState(false);
  const [flips, setFlips] = useState([]);
  const [running, setRunning] = useState(false);

  const doArm = useCallback(async () => {
    try {
      await setMode(2); // MODE_ROWHAMMER
      if (token.length !== 6) {
        Alert.alert('Bad token', 'Enter the 6-char token shown on the device OLED.');
        return;
      }
      const r = await arm(token);
      if (r.startsWith('OK')) {
        setArmed(true);
      } else {
        Alert.alert('Arm failed', r);
      }
    } catch (e) {
      Alert.alert('Error', e.message);
    }
  }, [setMode, arm, token]);

  const doHammer = useCallback(async () => {
    try {
      setRunning(true);
      const row = parseInt(aggressor, 10);
      const cnt = parseInt(count, 10);
      await hammerArm(row, cnt, pattern);
      const r = await hammerRun();
      // "OK FLIPS victim=R mask=0x..."
      const m = r.match(/OK FLIPS victim=(\d+) mask=0x([0-9A-Fa-f]+)/);
      if (m) {
        const vrow = parseInt(m[1], 10);
        const mask = parseInt(m[2], 16);
        setFlips((prev) => [{ vrow, mask, t: Date.now() }, ...prev]);
        if (mask === 0) {
          Alert.alert('No flips', `No bit-flips detected on victim row ${vrow}.`);
        } else {
          Alert.alert('Flips detected!', `Victim row ${vrow}, mask 0x${m[2]}`);
        }
      }
    } catch (e) {
      Alert.alert('Hammer error', e.message);
    } finally {
      setRunning(false);
    }
  }, [aggressor, count, pattern, hammerArm, hammerRun]);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Rowhammer Injection</Text>
        <Text style={styles.status}>
          {armed ? 'ARMED' : 'safe'} · {connected ? 'connected' : 'disconnected'}
        </Text>
      </View>

      <View style={styles.warn}>
        <Text style={styles.warnText}>
          ⚠️ Destructive operation. Requires two-key arm (physical button hold 3s
          + token from OLED). Use only on systems you own or are authorized to
          test. Author: jayis1.
        </Text>
      </View>

      <Text style={styles.label}>Aggressor row (decimal)</Text>
      <TextInput style={styles.input} keyboardType="numeric" value={aggressor} onChangeText={setAggressor} />

      <Text style={styles.label}>Hammer count</Text>
      <TextInput style={styles.input} keyboardType="numeric" value={count} onChangeText={setCount} />

      <Text style={styles.label}>Pattern</Text>
      {PATTERNS.map((p) => (
        <TouchableOpacity key={p.id} onPress={() => setPattern(p.id)}
          style={[styles.patternRow, pattern === p.id && styles.patternActive]}>
          <Text style={[styles.patternText, pattern === p.id && styles.patternTextActive]}>
            {p.name}
          </Text>
        </TouchableOpacity>
      ))}

      <Text style={styles.label}>Arm token (from device OLED)</Text>
      <TextInput style={styles.input} value={token} onChangeText={setToken}
        maxLength={6} placeholder="6-char token" autoCapitalize="characters" />

      <View style={styles.controls}>
        <Button title={armed ? 'Re-arm' : 'Arm'} color="#d29922" onPress={doArm} />
        <Button title="Disarm" color="#7d8590" onPress={async () => { await disarm(); setArmed(false); }} />
        <Button title={running ? 'Hammering...' : 'Run Hammer'} color="#d73a49"
          onPress={doHammer} disabled={!armed || running} />
      </View>

      <Text style={styles.sectionTitle}>Detected flips</Text>
      {flips.length === 0 ? (
        <Text style={styles.empty}>No flips recorded yet.</Text>
      ) : (
        flips.map((f, i) => (
          <View key={i} style={styles.flipRow}>
            <Text style={styles.flipText}>#{i + 1}</Text>
            <Text style={styles.flipText}>victim row: {f.vrow}</Text>
            <Text style={styles.flipText}>mask: 0x{f.mask.toString(16).padStart(8, '0')}</Text>
            <Text style={styles.flipText}>bits: {popcount(f.mask)}</Text>
            <Text style={styles.flipText}>{new Date(f.t).toLocaleTimeString()}</Text>
          </View>
        ))
      )}

      <Text style={styles.footer}>Author: jayis1 · DRAM-Phantom v1.0</Text>
    </ScrollView>
  );
}

function popcount(n) {
  let c = 0;
  while (n) { c += n & 1; n >>= 1; }
  return c;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 12 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  title: { color: '#e6edf3', fontSize: 18, fontWeight: 'bold' },
  status: { color: '#7d8590', fontSize: 12 },
  warn: { backgroundColor: '#3d1f1f', padding: 8, borderRadius: 4, marginBottom: 12, borderLeftWidth: 3, borderLeftColor: '#d73a49' },
  warnText: { color: '#ffa198', fontSize: 11 },
  label: { color: '#7d8590', fontSize: 12, marginTop: 8, marginBottom: 4 },
  input: { backgroundColor: '#161b22', color: '#e6edf3', borderRadius: 4, padding: 8, fontSize: 14, fontFamily: 'monospace' },
  patternRow: { padding: 8, backgroundColor: '#161b22', borderRadius: 4, marginBottom: 4 },
  patternActive: { backgroundColor: '#1f6feb' },
  patternText: { color: '#e6edf3', fontSize: 13 },
  patternTextActive: { color: '#fff', fontWeight: 'bold' },
  controls: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 12, marginBottom: 12 },
  sectionTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginTop: 8, marginBottom: 8 },
  empty: { color: '#484f58', fontSize: 12, fontStyle: 'italic' },
  flipRow: { flexDirection: 'row', gap: 12, padding: 8, backgroundColor: '#161b22', borderRadius: 4, marginBottom: 4 },
  flipText: { color: '#f0883e', fontSize: 12, fontFamily: 'monospace' },
  footer: { color: '#484f58', fontSize: 10, textAlign: 'center', marginTop: 16 },
});