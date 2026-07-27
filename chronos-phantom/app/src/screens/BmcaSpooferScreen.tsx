// src/screens/BmcaSpooferScreen.tsx — PTP Grandmaster spoofing control
//
// Author: jayis1
// License: GPL-2.0

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, TextInput, Alert } from 'react-native';
import { useBle } from '../ble/BleManager';
import { CMD, BmcaConfig } from '../types';
import { encodeBmcaConfig } from '../ble/protocol';

const CLOCK_CLASSES = [
  { value: 6, label: '6 — Primary Reference (GNSS/Atomic)' },
  { value: 7, label: '7 — PTP Boundary Clock' },
  { value: 13, label: '13 — Application-specific' },
  { value: 248, label: '248 — Default (ordinary)' },
  { value: 255, label: '255 — Unsynchronized' },
];

const CLOCK_ACCURACIES = [
  { value: 0x20, label: '0x20 — Sub-nanosecond (GNSS)' },
  { value: 0x21, label: '0x21 — 1 ns' },
  { value: 0x22, label: '0x22 — 2.5 ns' },
  { value: 0x23, label: '0x23 — 10 ns' },
  { value: 0x25, label: '0x25 — 100 ns' },
  { value: 0x31, label: '0x31 — 25 µs' },
  { value: 0x32, label: '0x32 — 100 µs' },
];

export default function BmcaSpooferScreen() {
  const { connected, sendCommand, status } = useBle();
  const [config, setConfig] = useState<BmcaConfig>({
    priority1: 0,
    clockClass: 6,
    clockAccuracy: 0x20,
    clockVariance: -4000,
    priority2: 0,
    domainNumber: 0,
    clockIdentity: '02:00:00:FF:FE:00:00:01',
  });

  const applySpoof = async () => {
    if (!connected) {
      Alert.alert('Not connected', 'Connect to device first');
      return;
    }
    await sendCommand(CMD.BMCA_SPOOF, encodeBmcaConfig(config));
    Alert.alert('BMCA Config Applied', `Priority1=${config.priority1}, Class=${config.clockClass}`);
  };

  const autoWin = async () => {
    if (!connected) return;
    await sendCommand(CMD.BMCA_AUTO_WIN, new Uint8Array(0));
    Alert.alert('Auto-Win Activated', 'Device will auto-adjust to beat observed GM');
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>BMCA Grandmaster Spoofer</Text>
      <Text style={styles.subtitle}>
        Configure the spoofed PTP Grandmaster attributes to win the
        Best Master Clock Algorithm election against the legitimate GM.
      </Text>

      <View style={styles.field}>
        <Text style={styles.fieldLabel}>grandmasterPriority1 (lower = better)</Text>
        <TextInput
          style={styles.input}
          value={config.priority1.toString()}
          keyboardType="numeric"
          onChangeText={(v) => setConfig({ ...config, priority1: parseInt(v) || 0 })}
        />
      </View>

      <View style={styles.field}>
        <Text style={styles.fieldLabel}>clockClass</Text>
        {CLOCK_CLASSES.map((c) => (
          <TouchableOpacity
            key={c.value}
            style={[styles.optionRow, config.clockClass === c.value && styles.optionActive]}
            onPress={() => setConfig({ ...config, clockClass: c.value })}
          >
            <Text style={[styles.optionText, config.clockClass === c.value && styles.optionTextActive]}>
              {c.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.field}>
        <Text style={styles.fieldLabel}>clockAccuracy</Text>
        {CLOCK_ACCURACIES.map((a) => (
          <TouchableOpacity
            key={a.value}
            style={[styles.optionRow, config.clockAccuracy === a.value && styles.optionActive]}
            onPress={() => setConfig({ ...config, clockAccuracy: a.value })}
          >
            <Text style={[styles.optionText, config.clockAccuracy === a.value && styles.optionTextActive]}>
              {a.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.field}>
        <Text style={styles.fieldLabel}>clockVariance (signed, more negative = better)</Text>
        <TextInput
          style={styles.input}
          value={config.clockVariance.toString()}
          keyboardType="numeric"
          onChangeText={(v) => setConfig({ ...config, clockVariance: parseInt(v) || 0 })}
        />
      </View>

      <View style={styles.field}>
        <Text style={styles.fieldLabel}>grandmasterPriority2</Text>
        <TextInput
          style={styles.input}
          value={config.priority2.toString()}
          keyboardType="numeric"
          onChangeText={(v) => setConfig({ ...config, priority2: parseInt(v) || 0 })}
        />
      </View>

      <View style={styles.field}>
        <Text style={styles.fieldLabel}>domainNumber</Text>
        <TextInput
          style={styles.input}
          value={config.domainNumber.toString()}
          keyboardType="numeric"
          onChangeText={(v) => setConfig({ ...config, domainNumber: parseInt(v) || 0 })}
        />
      </View>

      <TouchableOpacity style={styles.buttonPrimary} onPress={applySpoof}>
        <Text style={styles.buttonText}>Apply BMCA Spoof</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.buttonSecondary} onPress={autoWin}>
        <Text style={styles.buttonText}>Auto-Win (beat observed GM)</Text>
      </TouchableOpacity>

      {status && (
        <View style={styles.comparison}>
          <Text style={styles.comparisonTitle}>BMCA Comparison</Text>
          <Text style={styles.comparisonRow}>
            Observed GM: priority1={status.observedGmPriority1}, class={status.observedGmClockClass}
          </Text>
          <Text style={styles.comparisonRow}>
            Our spoof:  priority1={status.spoofedGmPriority1}, class={status.spoofedGmClockClass}
          </Text>
          {status.spoofedGmPriority1 <= status.observedGmPriority1 && (
            <Text style={styles.winText}>✅ We would WIN the BMCA election</Text>
          )}
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a' },
  content: { padding: 16, paddingBottom: 40 },
  title: { fontSize: 20, fontWeight: 'bold', color: '#00ff88' },
  subtitle: { fontSize: 12, color: '#666', marginTop: 4, marginBottom: 16 },
  field: { marginBottom: 20 },
  fieldLabel: { fontSize: 12, color: '#888', marginBottom: 8 },
  input: {
    backgroundColor: '#1a1a1a', borderRadius: 8, padding: 12,
    color: '#ccc', fontSize: 14, fontFamily: 'monospace',
    borderWidth: 1, borderColor: '#333',
  },
  optionRow: {
    backgroundColor: '#1a1a1a', borderRadius: 6, padding: 12, marginBottom: 4,
    borderWidth: 1, borderColor: '#333',
  },
  optionActive: { borderColor: '#00ff88', backgroundColor: '#0a2a0a' },
  optionText: { color: '#999', fontSize: 12 },
  optionTextActive: { color: '#00ff88' },
  buttonPrimary: {
    backgroundColor: '#00aa44', borderRadius: 8, padding: 14,
    alignItems: 'center', marginTop: 8,
  },
  buttonSecondary: {
    backgroundColor: '#333', borderRadius: 8, padding: 14,
    alignItems: 'center', marginTop: 8,
  },
  buttonText: { color: 'white', fontSize: 14, fontWeight: '600' },
  comparison: {
    backgroundColor: '#1a1a1a', borderRadius: 8, padding: 16, marginTop: 20,
    borderWidth: 1, borderColor: '#333',
  },
  comparisonTitle: { fontSize: 14, color: '#00ff88', fontWeight: '600', marginBottom: 8 },
  comparisonRow: { fontSize: 11, color: '#999', fontFamily: 'monospace', marginBottom: 4 },
  winText: { color: '#00ff88', fontSize: 13, marginTop: 8, fontWeight: '600' },
});

// Author: jayis1
// License: GPL-2.0