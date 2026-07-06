// lumen-tap/app/screens/LiveScreen.js
// Real-time scope + spectrogram + status readouts.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React, { useState, useEffect, useContext } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Scope, SpectrumBars } from '../components/Scope';
import { DEMOD_LABELS, SD_STATE_LABELS, safetyVerdict, dutyToMw } from '../utils/protocol';
import { DeviceContext } from '../App';

export function LiveScreen() {
  const { status, syntheticSamples } = useContext(DeviceContext);
  const verdict = safetyVerdict(status);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.h1}>LIVE</Text>

      <Scope samples={syntheticSamples} label="DEMOD OUT (48 kHz)" color="#39FF14" />
      <SpectrumBars rms={status?.rms_out || 0} snrDb={status?.snr_db || 0} />

      <View style={styles.grid}>
        <Stat label="LASER"   value={status?.laser ? 'ON' : 'OFF'}
              color={status?.laser ? '#FF4040' : '#888'} />
        <Stat label="POWER"   value={`${(((status?.pwm||0)/255)*100).toFixed(0)}%`} />
        <Stat label="≈ mW"    value={dutyToMw(status?.pwm||0).toFixed(2)} />
        <Stat label="ARM KEY" value={status?.arm ? 'IN' : 'OUT'}
              color={status?.arm ? '#39FF14' : '#FFA500'} />
        <Stat label="AMBIENT" value={`${status?.ambient ?? '--'}`} />
        <Stat label="SNR"     value={`${(status?.snr_db ?? 0).toFixed(1)} dB`} />
        <Stat label="RMS IN"  value={(status?.rms_in ?? 0).toFixed(4)} />
        <Stat label="RMS OUT" value={(status?.rms_out ?? 0).toFixed(4)} />
        <Stat label="DEMOD"   value={DEMOD_LABELS[status?.demod ?? 0]} />
        <Stat label="BAND"    value={`${(status?.bp_lo??0).toFixed(0)}–${(status?.bp_hi??0).toFixed(0)} Hz`} />
        <Stat label="NOISE"   value={((status?.noise ?? 0) * 100).toFixed(0) + '%'} />
        <Stat label="GAIN"    value={(status?.gain ?? 1).toFixed(2) + 'x'} />
        <Stat label="SD"      value={SD_STATE_LABELS[status?.sd_state ?? 0]} />
        <Stat label="SD BLK"  value={`${status?.sd_blk ?? 0}`} />
      </View>

      <View style={[styles.verdict, {borderColor: verdict.canEmit ? '#39FF14' : '#FF4040'}]}>
        <Text style={{color: verdict.canEmit ? '#39FF14' : '#FF4040', fontFamily:'monospace'}}>
          {verdict.canEmit ? 'EMISSION PERMITTED' : 'BLOCKED: ' + verdict.reasons.join(', ')}
        </Text>
      </View>

      <Text style={styles.foot}>
        ⚖️ Authorized security research only. Never point at people, vehicles,
        or aircraft. Class 1 emission cap enforced in firmware.
      </Text>
    </ScrollView>
  );
}

function Stat({label, value, color = '#ccc'}) {
  return (
    <View style={styles.stat}>
      <Text style={styles.statLabel}>{label}</Text>
      <Text style={[styles.statValue, {color}]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  h1: { color: '#39FF14', fontSize: 20, fontFamily: 'monospace', marginBottom: 8 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  stat: { width: '48%', backgroundColor: '#161616', padding: 8, borderRadius: 4, marginVertical: 3 },
  statLabel: { color: '#666', fontSize: 10, fontFamily: 'monospace' },
  statValue: { color: '#ccc', fontSize: 16, fontFamily: 'monospace' },
  verdict: { borderWidth: 1, borderRadius: 4, padding: 10, marginVertical: 10, alignItems: 'center' },
  foot: { color: '#555', fontSize: 10, fontFamily: 'monospace', marginTop: 12, fontStyle: 'italic' },
});