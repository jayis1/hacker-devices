/**
 * DashboardScreen.js — modality, gain, sample rate, trigger, battery overview
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function DashboardScreen({ navigation }) {
  const { sendCommand, status, disconnect } = useDevice();
  const [modality, setModality] = useState('POWER');
  const [gain, setGain] = useState(20);
  const [rate, setRate] = useState(1000000);
  const [nTraces, setNTraces] = useState(5000);
  const [model, setModel] = useState('SBOX');
  const [trigger, setTrigger] = useState('ANALOG');
  const [ptMode, setPtMode] = useState('RANDOM');
  const [info, setInfo] = useState('');

  const refresh = useCallback(async () => {
    const resp = await sendCommand('STATUS', 3000);
    if (resp) setInfo(resp);
  }, [sendCommand]);

  useEffect(() => { refresh(); }, [refresh]);

  const setAndSend = async (cmd) => {
    const resp = await sendCommand(cmd, 2000);
    if (resp && resp.startsWith('OK')) {
      // parse updated values
    } else if (resp) {
      Alert.alert('Error', resp);
    }
    refresh();
  };

  const toggleModality = () => {
    const next = modality === 'POWER' ? 'EM' : 'POWER';
    setModality(next);
    setAndSend(`SET MODALITY ${next}`);
  };

  const adjustGain = (delta) => {
    const g = Math.max(-20, Math.min(68, gain + delta));
    setGain(g);
    setAndSend(`SET GAIN ${g}`);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>SideProbe Dashboard</Text>
        <Text style={styles.status}>{status}</Text>
      </View>

      <Text style={styles.infoLine}>{info}</Text>

      {/* Modality */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Modality</Text>
        <TouchableOpacity style={styles.modalityBtn} onPress={toggleModality}>
          <Text style={styles.modalityText}>
            {modality === 'POWER' ? '⚡ Power (shunt)' : '📡 EM (near-field)'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Gain */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Gain: {gain} dB</Text>
        <View style={styles.row}>
          <TouchableOpacity style={styles.smallBtn} onPress={() => adjustGain(-6)}>
            <Text style={styles.btnText}>−6 dB</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.smallBtn} onPress={() => adjustGain(6)}>
            <Text style={styles.btnText}>+6 dB</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.smallBtn} onPress={async () => { await sendCommand('CAL', 5000); refresh(); }}>
            <Text style={styles.btnText}>Auto-Cal</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Sample rate */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Sample Rate: {(rate/1000).toFixed(0)} kSPS</Text>
        <View style={styles.row}>
          {[1000000, 500000, 250000, 100000].map((r) => (
            <TouchableOpacity key={r} style={[styles.smallBtn, rate===r && styles.activeBtn]}
              onPress={() => { setRate(r); setAndSend(`SET RATE ${r}`); }}>
              <Text style={styles.btnText}>{r/1000}k</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* N traces */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Traces: {nTraces}</Text>
        <View style={styles.row}>
          {[1000, 5000, 10000, 20000].map((n) => (
            <TouchableOpacity key={n} style={[styles.smallBtn, nTraces===n && styles.activeBtn]}
              onPress={() => { setNTraces(n); setAndSend(`SET NTRACES ${n}`); }}>
              <Text style={styles.btnText}>{n}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Model */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Leakage Model</Text>
        <View style={styles.row}>
          {['SBOX', 'HW', 'HD'].map((m) => (
            <TouchableOpacity key={m} style={[styles.smallBtn, model===m && styles.activeBtn]}
              onPress={() => { setModel(m); setAndSend(`SET MODEL ${m}`); }}>
              <Text style={styles.btnText}>{m}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Trigger */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Trigger Source</Text>
        <View style={styles.row}>
          {['ANALOG', 'GPIO', 'UART', 'FREE'].map((t) => (
            <TouchableOpacity key={t} style={[styles.smallBtn, trigger===t && styles.activeBtn]}
              onPress={() => { setTrigger(t); setAndSend(`SET TRIG ${t}`); }}>
              <Text style={styles.btnText}>{t}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Plaintext mode */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Plaintext Mode</Text>
        <View style={styles.row}>
          {['RANDOM', 'DRIVE', 'LISTEN'].map((p) => (
            <TouchableOpacity key={p} style={[styles.smallBtn, ptMode===p && styles.activeBtn]}
              onPress={() => { setPtMode(p); setAndSend(`SET PTMODE ${p}`); }}>
              <Text style={styles.btnText}>{p}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Navigation */}
      <View style={styles.navRow}>
        <TouchableOpacity style={styles.navBtn} onPress={() => navigation.navigate('Capture')}>
          <Text style={styles.navBtnText}>📡 Capture</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navBtn} onPress={() => navigation.navigate('TraceViewer')}>
          <Text style={styles.navBtnText}>📊 Traces</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navBtnAttack} onPress={() => navigation.navigate('Attack')}>
          <Text style={styles.navBtnText}>🔐 Attack</Text>
        </TouchableOpacity>
      </View>
      <View style={styles.navRow}>
        <TouchableOpacity style={styles.navBtn} onPress={() => navigation.navigate('PlaintextManager')}>
          <Text style={styles.navBtnText}>📝 Plaintexts</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navBtn} onPress={() => navigation.navigate('Settings')}>
          <Text style={styles.navBtnText}>⚙️ Settings</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navBtnDanger} onPress={disconnect}>
          <Text style={styles.navBtnText}>🔌 Disconnect</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35' },
  status: { color: '#58a6ff', fontSize: 12 },
  infoLine: { color: '#8b949e', fontSize: 11, marginBottom: 12, fontFamily: 'monospace' },
  card: { backgroundColor: '#161b22', borderRadius: 10, padding: 14, marginBottom: 10, borderWidth: 1, borderColor: '#30363d' },
  cardTitle: { color: '#e6edf3', fontSize: 15, fontWeight: 'bold', marginBottom: 8 },
  modalityBtn: { backgroundColor: '#21262d', borderRadius: 8, padding: 12, alignItems: 'center' },
  modalityText: { color: '#ff6b35', fontSize: 16, fontWeight: 'bold' },
  row: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  smallBtn: { backgroundColor: '#21262d', padding: 10, borderRadius: 6, minWidth: 60, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  activeBtn: { backgroundColor: '#ff6b35', borderColor: '#ff6b35' },
  btnText: { color: '#e6edf3', fontSize: 13, fontWeight: '600' },
  navRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 10 },
  navBtn: { flex: 1, backgroundColor: '#161b22', padding: 14, borderRadius: 8, marginHorizontal: 4, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  navBtnAttack: { flex: 1, backgroundColor: '#da3633', padding: 14, borderRadius: 8, marginHorizontal: 4, alignItems: 'center' },
  navBtnDanger: { flex: 1, backgroundColor: '#6e2c2c', padding: 14, borderRadius: 8, marginHorizontal: 4, alignItems: 'center' },
  navBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
});