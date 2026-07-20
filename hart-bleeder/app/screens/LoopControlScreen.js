/**
 * hart-bleeder — LoopControlScreen.js
 * Physical-layer loop control: passive/inject/clamp/sag/open modes,
 * live loop current & voltage readout, and current clamping slider.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, SafeAreaView,
  StatusBar, Alert, Slider,
} from 'react-native';

const MODES = [
  { id: 0, name: 'PASSIVE',  desc: 'Transparent monitor only', color: '#10b981' },
  { id: 1, name: 'INJECT',   desc: 'Inject HART frames',      color: '#f59e0b' },
  { id: 2, name: 'CLAMP',    desc: 'Force loop current',       color: '#e94560' },
  { id: 3, name: 'SAG',      desc: 'Induce voltage sag',       color: '#e94560' },
  { id: 4, name: 'OPEN',     desc: 'Break loop continuity (DoS)', color: '#e94560' },
  { id: 5, name: 'COVERT',   desc: 'Low-current covert channel', color: '#4a90d9' },
];

export default function LoopControlScreen({ bleManager, connected, loopCurrent, armed }) {
  const [mode, setMode] = useState(0);
  const [clampMa, setClampMa] = useState(4);
  const [sagPct, setSagPct] = useState(50);
  const [sagMs, setSagMs] = useState(500);

  const setModeRemote = async (m) => {
    if (!connected) { Alert.alert('Not connected'); return; }
    if (m >= 2 && !armed) {
      Alert.alert('Not Armed', 'Destructive modes require arming from the Dashboard.');
      return;
    }
    try {
      await bleManager.setMode(m);
      setMode(m);
    } catch (e) { Alert.alert('Error', e.message); }
  };

  const applyClamp = async () => {
    if (!armed) { Alert.alert('Not Armed'); return; }
    try { await bleManager.spoofPV(clampMa); }
    catch (e) { Alert.alert('Error', e.message); }
  };

  const fireSag = async () => {
    if (!armed) { Alert.alert('Not Armed'); return; }
    Alert.alert('Confirm Sag', `Induce ${sagPct}% sag for ${sagMs}ms?`, [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Fire', style: 'destructive', onPress: async () => {
        try { await bleManager.sagInject(sagMs, sagPct); }
        catch (e) { Alert.alert('Error', e.message); }
      }},
    ]);
  };

  const fireOpen = async () => {
    if (!armed) { Alert.alert('Not Armed'); return; }
    Alert.alert('Confirm Open-Loop', 'Break loop continuity? This may trip safety systems.', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Open Loop', style: 'destructive', onPress: async () => {
        try { await bleManager.loopDoS(sagMs); }
        catch (e) { Alert.alert('Error', e.message); }
      }},
    ]);
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <Text style={styles.title}>Loop Control</Text>

      <View style={styles.readout}>
        <View style={styles.readoutItem}>
          <Text style={styles.readoutLabel}>LOOP CURRENT</Text>
          <Text style={styles.readoutValue}>{loopCurrent.toFixed(2)} mA</Text>
        </View>
        <View style={styles.readoutBar}>
          <View style={[styles.readoutFill, {
            width: `${Math.min(100, Math.max(0, ((loopCurrent - 4) / 16) * 100))}%`,
          }]} />
        </View>
        <Text style={styles.readoutScale}>4 mA (0%) ───── 20 mA (100%)</Text>
      </View>

      <Text style={styles.sectionLabel}>Mode Selection</Text>
      {MODES.map((m) => (
        <TouchableOpacity
          key={m.id}
          style={[styles.modeRow, mode === m.id && { borderColor: m.color, borderWidth: 2 }]}
          onPress={() => m.id === 4 ? fireOpen() : setModeRemote(m.id)}
        >
          <View style={[styles.modeDot, { backgroundColor: m.color }]} />
          <View style={styles.modeText}>
            <Text style={styles.modeName}>{m.name}</Text>
            <Text style={styles.modeDesc}>{m.desc}</Text>
          </View>
        </TouchableOpacity>
      ))}

      <Text style={styles.sectionLabel}>Current Clamp (PV Spoofing)</Text>
      <Text style={styles.sliderValue}>{clampMa.toFixed(1)} mA</Text>
      <Slider
        style={styles.slider}
        minimumValue={3.8}
        maximumValue={22}
        step={0.1}
        value={clampMa}
        onValueChange={setClampMa}
        minimumTrackTintColor="#e94560"
        maximumTrackTintColor="#333"
        onSlidingComplete={applyClamp}
      />

      <Text style={styles.sectionLabel}>Voltage Sag Injection</Text>
      <Text style={styles.sliderValue}>{sagPct}% for {sagMs}ms</Text>
      <Slider
        style={styles.slider}
        minimumValue={0}
        maximumValue={100}
        step={5}
        value={sagPct}
        onValueChange={setSagPct}
        minimumTrackTintColor="#f59e0b"
        maximumTrackTintColor="#333"
      />
      <Slider
        style={styles.slider}
        minimumValue={10}
        maximumValue={2000}
        step={10}
        value={sagMs}
        onValueChange={setSagMs}
        minimumTrackTintColor="#f59e0b"
        maximumTrackTintColor="#333"
      />
      <TouchableOpacity style={styles.fireBtn} onPress={fireSag}>
        <Text style={styles.fireBtnText}>Fire Sag</Text>
      </TouchableOpacity>

      <Text style={styles.disclaimer}>
        ⚠ Loop manipulation can disrupt industrial processes, trip safety
        systems, and cause physical harm. Authorized testing only.
      </Text>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 14 },
  title: { color: '#e94560', fontSize: 20, fontWeight: 'bold', marginBottom: 12 },
  readout: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginBottom: 16 },
  readoutItem: { flexDirection: 'row', justifyContent: 'space-between' },
  readoutLabel: { color: '#888', fontSize: 11 },
  readoutValue: { color: '#10b981', fontSize: 22, fontWeight: 'bold' },
  readoutBar: { height: 8, backgroundColor: '#2d2d44', borderRadius: 4, marginTop: 10 },
  readoutFill: { height: 8, backgroundColor: '#10b981', borderRadius: 4 },
  readoutScale: { color: '#555', fontSize: 9, marginTop: 4, textAlign: 'center' },
  sectionLabel: { color: '#aaa', fontSize: 13, fontWeight: 'bold', marginTop: 10, marginBottom: 6 },
  modeRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 6, borderWidth: 1, borderColor: '#2d2d44' },
  modeDot: { width: 12, height: 12, borderRadius: 6, marginRight: 12 },
  modeText: { flex: 1 },
  modeName: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
  modeDesc: { color: '#777', fontSize: 10 },
  sliderValue: { color: '#e94560', fontSize: 13, fontWeight: 'bold', marginBottom: 4 },
  slider: { width: '100%', height: 40, marginBottom: 6 },
  fireBtn: { backgroundColor: '#e94560', padding: 14, borderRadius: 10, alignItems: 'center', marginTop: 8 },
  fireBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  disclaimer: { color: '#f59e0b', fontSize: 9, textAlign: 'center', marginTop: 12 },
});