/**
 * CaptureScreen.js — live trace capture & waveform preview
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Sends "CAPTURE ONE" to the device, parses the streamed downsampled trace
 * (256 16-bit samples as hex prefixed with 'T'), and renders it as a
 * scrolling waveform using a simple canvas-like View of bars.
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

function parseTraceLine(line) {
  if (!line || line[0] !== 'T') return null;
  const hex = line.slice(1);
  const samples = [];
  for (let i = 0; i + 4 <= hex.length; i += 4) {
    const v = parseInt(hex.slice(i, i + 4), 16);
    samples.push(v);
  }
  return samples;
}

function TraceWaveform({ samples }) {
  if (!samples || samples.length === 0) return null;
  const maxV = 0xFFFF;
  return (
    <View style={styles.waveform}>
      {samples.map((v, i) => {
        const h = Math.abs((v - 0x8000) / maxV) * 120;
        return (
          <View key={i} style={[styles.bar, {
            left: i * 1.3,
            height: Math.max(2, h),
            top: 70 - h / 2,
            backgroundColor: v > 0x8000 ? '#58a6ff' : '#ff6b35',
          }]} />
        );
      })}
    </View>
  );
}

export default function CaptureScreen({ navigation }) {
  const { sendCommand } = useDevice();
  const [samples, setSamples] = useState(null);
  const [capturing, setCapturing] = useState(false);
  const [stats, setStats] = useState('');

  const captureOne = useCallback(async () => {
    setCapturing(true);
    const resp = await sendCommand('CAPTURE ONE', 10000);
    setCapturing(false);
    if (resp && resp[0] === 'T') {
      const s = parseTraceLine(resp);
      setSamples(s);
      // Compute simple stats
      if (s && s.length > 0) {
        const mean = s.reduce((a, b) => a + b, 0) / s.length;
        const variance = s.reduce((a, b) => a + (b - mean) ** 2, 0) / s.length;
        const rms = Math.sqrt(variance);
        const minV = Math.min(...s);
        const maxV = Math.max(...s);
        setStats(`n=${s.length} mean=0x${mean.toFixed(0)} rms=${rms.toFixed(0)} min=0x${minV.toString(16)} max=0x${maxV.toString(16)}`);
      }
    } else if (resp) {
      Alert.alert('Capture', resp);
    } else {
      Alert.alert('Capture', 'No response (timeout)');
    }
  }, [sendCommand]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Live Capture</Text>
      <Text style={styles.hint}>Single-trigger capture for preview & alignment.</Text>

      <TouchableOpacity style={styles.captureBtn} onPress={captureOne} disabled={capturing}>
        <Text style={styles.captureBtnText}>{capturing ? 'Capturing...' : '📡 Capture One Trace'}</Text>
      </TouchableOpacity>

      <View style={styles.traceBox}>
        <Text style={styles.axisLabel}>amplitude</Text>
        {samples ? <TraceWaveform samples={samples} /> : <Text style={styles.placeholder}>No trace yet</Text>}
        <Text style={styles.axisLabel}>sample #  →</Text>
      </View>

      {stats ? <Text style={styles.stats}>{stats}</Text> : null}

      <TouchableOpacity style={styles.navBtn} onPress={() => navigation.navigate('Attack')}>
        <Text style={styles.navBtnText}>→ Run CPA Attack</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35', marginBottom: 4 },
  hint: { color: '#8b949e', fontSize: 13, marginBottom: 16 },
  captureBtn: { backgroundColor: '#ff6b35', padding: 16, borderRadius: 10, alignItems: 'center', marginBottom: 16 },
  captureBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  traceBox: { backgroundColor: '#161b22', borderRadius: 10, padding: 12, height: 180, borderWidth: 1, borderColor: '#30363d', position: 'relative' },
  axisLabel: { color: '#6e7681', fontSize: 10, fontFamily: 'monospace' },
  placeholder: { color: '#6e7681', textAlign: 'center', marginTop: 60 },
  waveform: { position: 'absolute', left: 12, right: 12, top: 20, bottom: 20 },
  bar: { position: 'absolute', width: 1.2, backgroundColor: '#58a6ff' },
  stats: { color: '#58a6ff', fontSize: 12, fontFamily: 'monospace', marginTop: 12, marginBottom: 12 },
  navBtn: { backgroundColor: '#161b22', padding: 14, borderRadius: 8, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  navBtnText: { color: '#ff6b35', fontWeight: 'bold', fontSize: 14 },
});