// src/screens/AttackControlScreen.tsx — Attack mode and skew control
//
// Author: jayis1
// License: GPL-2.0

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import { useBle } from '../ble/BleManager';
import SkewProfilePicker from '../components/SkewProfilePicker';
import { CMD, OpMode, SkewConfig, SkewPreset } from '../types';
import { encodeMode, encodePreset, encodeSkewConfig } from '../ble/protocol';

const MODES: { id: OpMode; label: string; desc: string; icon: string }[] = [
  { id: 'passive', label: 'Passive Sniff', desc: 'Listen only — map topology', icon: '👁' },
  { id: 'inline', label: 'Inline MITM', desc: 'Intercept & modify PTP frames', icon: '⚡' },
  { id: 'rogue_gm', label: 'Rogue Grandmaster', desc: 'Spoof PTP GM, win BMCA', icon: '👑' },
  { id: 'transparent', label: 'Transparent', desc: 'Pass-through (safe mode)', icon: '🛡' },
];

const PRESETS: { id: SkewPreset; label: string; desc: string }[] = [
  { id: 'kerberos_ext', label: 'Kerberos +5min', desc: 'Extend ticket validity' },
  { id: 'kerberos_replay', label: 'Kerberos -10min', desc: 'Replay expired tickets' },
  { id: 'pmu_slow', label: 'PMU Slow Drift', desc: '0.5 ppm — desync over hours' },
  { id: 'pmu_sawtooth', label: 'PMU Sawtooth', desc: '±1 ms, 60s period' },
  { id: 'jitter_100us', label: 'Jitter ±100µs', desc: 'Disrupt phase-sensitive apps' },
];

export default function AttackControlScreen() {
  const { connected, sendCommand, status } = useBle();
  const [selectedMode, setSelectedMode] = useState<OpMode>('passive');
  const [skewActive, setSkewActive] = useState(false);
  const [customConfig, setCustomConfig] = useState<SkewConfig>({
    profile: 'step',
    offsetNs: 0,
    rateNsps: 0,
    jitterAmpNs: 100000,
    sawtoothPeriodMs: 60000,
    active: false,
  });

  const setMode = async (mode: OpMode) => {
    setSelectedMode(mode);
    if (connected) {
      await sendCommand(CMD.SET_MODE, encodeMode(mode));
    }
  };

  const applyPreset = async (preset: SkewPreset) => {
    if (!connected) {
      Alert.alert('Not connected', 'Connect to device first');
      return;
    }
    await sendCommand(CMD.SET_SKEW_PRESET, encodePreset(preset));
    setSkewActive(true);
    Alert.alert('Preset Applied', `Skew preset "${preset}" activated`);
  };

  const stopSkew = async () => {
    const cfg = { ...customConfig, active: false };
    setCustomConfig(cfg);
    setSkewActive(false);
    if (connected) await sendCommand(CMD.SET_SKEW_PROFILE, encodeSkewConfig(cfg));
  };

  const applyCustom = async () => {
    const cfg = { ...customConfig, active: true };
    setCustomConfig(cfg);
    setSkewActive(true);
    if (connected) await sendCommand(CMD.SET_SKEW_PROFILE, encodeSkewConfig(cfg));
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.sectionTitle}>Operating Mode</Text>
      {MODES.map((m) => (
        <TouchableOpacity
          key={m.id}
          style={[styles.modeCard, selectedMode === m.id && styles.modeCardActive]}
          onPress={() => setMode(m.id)}
        >
          <Text style={styles.modeIcon}>{m.icon}</Text>
          <View style={styles.modeInfo}>
            <Text style={styles.modeLabel}>{m.label}</Text>
            <Text style={styles.modeDesc}>{m.desc}</Text>
          </View>
          {selectedMode === m.id && <Text style={styles.checkMark}>✓</Text>}
        </TouchableOpacity>
      ))}

      <Text style={styles.sectionTitle}>Skew Presets</Text>
      {PRESETS.map((p) => (
        <TouchableOpacity key={p.id} style={styles.presetCard} onPress={() => applyPreset(p.id)}>
          <View>
            <Text style={styles.presetLabel}>{p.label}</Text>
            <Text style={styles.presetDesc}>{p.desc}</Text>
          </View>
          <Text style={styles.applyArrow}>→</Text>
        </TouchableOpacity>
      ))}

      <Text style={styles.sectionTitle}>Custom Skew</Text>
      <SkewProfilePicker config={customConfig} onChange={setCustomConfig} />
      <View style={styles.buttonRow}>
        <TouchableOpacity style={[styles.button, styles.buttonPrimary]} onPress={applyCustom}>
          <Text style={styles.buttonText}>Apply</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.button, styles.buttonDanger]} onPress={stopSkew}>
          <Text style={styles.buttonText}>Stop</Text>
        </TouchableOpacity>
      </View>

      {skewActive && (
        <View style={styles.warningCard}>
          <Text style={styles.warningText}>
            ⚠️ SKEW ACTIVE — Target clocks are being manipulated!
          </Text>
        </View>
      )}

      {status && (
        <View style={styles.statusCard}>
          <Text style={styles.statusText}>
            Current skew: {status.currentSkewNsStr} ns | Mode: {status.mode}
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a' },
  content: { padding: 16, paddingBottom: 40 },
  sectionTitle: { fontSize: 16, color: '#00ff88', marginTop: 20, marginBottom: 8, fontWeight: '600' },
  modeCard: {
    flexDirection: 'row', alignItems: 'center', backgroundColor: '#1a1a1a',
    borderRadius: 10, padding: 16, marginBottom: 8, borderWidth: 1, borderColor: '#333',
  },
  modeCardActive: { borderColor: '#00ff88', backgroundColor: '#0a2a0a' },
  modeIcon: { fontSize: 24, marginRight: 16 },
  modeInfo: { flex: 1 },
  modeLabel: { fontSize: 14, color: '#ccc', fontWeight: '600' },
  modeDesc: { fontSize: 11, color: '#666', marginTop: 2 },
  checkMark: { color: '#00ff88', fontSize: 20, fontWeight: 'bold' },
  presetCard: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    backgroundColor: '#1a1a1a', borderRadius: 10, padding: 14, marginBottom: 8,
  },
  presetLabel: { fontSize: 14, color: '#ccc' },
  presetDesc: { fontSize: 11, color: '#666', marginTop: 2 },
  applyArrow: { color: '#00ff88', fontSize: 20 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 16 },
  button: { paddingHorizontal: 32, paddingVertical: 12, borderRadius: 8 },
  buttonPrimary: { backgroundColor: '#00aa44' },
  buttonDanger: { backgroundColor: '#aa3333' },
  buttonText: { color: 'white', fontSize: 14, fontWeight: '600' },
  warningCard: {
    backgroundColor: '#3a1a00', borderRadius: 8, padding: 12, marginTop: 16,
    borderWidth: 1, borderColor: '#ff8800',
  },
  warningText: { color: '#ff8800', fontSize: 12, textAlign: 'center', fontWeight: '600' },
  statusCard: { backgroundColor: '#1a1a1a', borderRadius: 8, padding: 12, marginTop: 16 },
  statusText: { color: '#888', fontSize: 11, fontFamily: 'monospace' },
});

// Author: jayis1
// License: GPL-2.0