// src/components/SkewProfilePicker.tsx — Skew profile configuration component
//
// Author: jayis1
// License: GPL-2.0

import React from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity } from 'react-native';
import { SkewConfig, SkewProfile } from '../types';

interface SkewProfilePickerProps {
  config: SkewConfig;
  onChange: (config: SkewConfig) => void;
}

const PROFILES: { id: SkewProfile; label: string; desc: string }[] = [
  { id: 'step', label: 'Step', desc: 'Instantaneous offset' },
  { id: 'ramp', label: 'Ramp', desc: 'Linear drift over time' },
  { id: 'stealth', label: 'Stealth', desc: 'Sub-ppm drift (evade detection)' },
  { id: 'jitter', label: 'Jitter', desc: 'Pseudo-random ±amplitude' },
  { id: 'sawtooth', label: 'Sawtooth', desc: 'Periodic ramp-and-reset' },
];

export default function SkewProfilePicker({ config, onChange }: SkewProfilePickerProps) {
  return (
    <View style={styles.container}>
      <Text style={styles.label}>Profile</Text>
      <View style={styles.profileRow}>
        {PROFILES.map((p) => (
          <TouchableOpacity
            key={p.id}
            style={[styles.profileBtn, config.profile === p.id && styles.profileBtnActive]}
            onPress={() => onChange({ ...config, profile: p.id })}
          >
            <Text style={[styles.profileBtnText, config.profile === p.id && styles.profileBtnTextActive]}>
              {p.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {(config.profile === 'step' || config.profile === 'ramp' || config.profile === 'jitter' || config.profile === 'sawtooth') && (
        <View style={styles.field}>
          <Text style={styles.fieldLabel}>Offset (ns)</Text>
          <TextInput
            style={styles.input}
            value={config.offsetNs.toString()}
            keyboardType="numeric"
            onChangeText={(v) => onChange({ ...config, offsetNs: parseInt(v) || 0 })}
          />
        </View>
      )}

      {(config.profile === 'ramp' || config.profile === 'stealth') && (
        <View style={styles.field}>
          <Text style={styles.fieldLabel}>Rate (ns/s) — ppb drift</Text>
          <TextInput
            style={styles.input}
            value={config.rateNsps.toString()}
            keyboardType="numeric"
            onChangeText={(v) => onChange({ ...config, rateNsps: parseInt(v) || 0 })}
          />
        </View>
      )}

      {config.profile === 'jitter' && (
        <View style={styles.field}>
          <Text style={styles.fieldLabel}>Jitter Amplitude (ns)</Text>
          <TextInput
            style={styles.input}
            value={config.jitterAmpNs.toString()}
            keyboardType="numeric"
            onChangeText={(v) => onChange({ ...config, jitterAmpNs: parseInt(v) || 0 })}
          />
        </View>
      )}

      {config.profile === 'sawtooth' && (
        <View style={styles.field}>
          <Text style={styles.fieldLabel}>Sawtooth Period (ms)</Text>
          <TextInput
            style={styles.input}
            value={config.sawtoothPeriodMs.toString()}
            keyboardType="numeric"
            onChangeText={(v) => onChange({ ...config, sawtoothPeriodMs: parseInt(v) || 0 })}
          />
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { backgroundColor: '#1a1a1a', borderRadius: 10, padding: 14 },
  label: { fontSize: 12, color: '#888', marginBottom: 8 },
  profileRow: { flexDirection: 'row', flexWrap: 'wrap', marginBottom: 12 },
  profileBtn: {
    backgroundColor: '#0a0a0a', borderRadius: 6, paddingHorizontal: 12,
    paddingVertical: 8, marginRight: 6, marginBottom: 6, borderWidth: 1, borderColor: '#333',
  },
  profileBtnActive: { borderColor: '#00ff88', backgroundColor: '#0a2a0a' },
  profileBtnText: { color: '#999', fontSize: 12 },
  profileBtnTextActive: { color: '#00ff88', fontWeight: '600' },
  field: { marginBottom: 12 },
  fieldLabel: { fontSize: 11, color: '#666', marginBottom: 4 },
  input: {
    backgroundColor: '#0a0a0a', borderRadius: 6, padding: 10,
    color: '#ccc', fontSize: 13, fontFamily: 'monospace',
    borderWidth: 1, borderColor: '#333',
  },
});

// Author: jayis1
// License: GPL-2.0