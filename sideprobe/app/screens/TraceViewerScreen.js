/**
 * TraceViewerScreen.js — browse & overlay stored traces
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * In a full deployment, traces are fetched from the device's SD card over USB
 * bulk. Over BLE we fetch a downsampled preview of the last few captured
 * traces. This screen renders an overlay of multiple traces so the operator
 * can visually inspect trigger alignment and leakage shape.
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, TextInput } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const COLORS = ['#58a6ff', '#ff6b35', '#3fb950', '#f0883e', '#bc8cff', '#da3633'];

export default function TraceViewerScreen() {
  const { sendCommand } = useDevice();
  const [traceIdx, setTraceIdx] = useState('0');
  const [count, setCount] = useState('3');
  const [traces, setTraces] = useState([]);
  const [loading, setLoading] = useState(false);

  const fetchTraces = useCallback(async () => {
    setLoading(true);
    const fetched = [];
    const n = Math.max(1, Math.min(6, parseInt(count, 10) || 1));
    const start = Math.max(0, parseInt(traceIdx, 10) || 0);
    for (let i = 0; i < n; i++) {
      const resp = await sendCommand(`DUMP ${start + i}`, 8000);
      if (resp && resp[0] === 'T') {
        const hex = resp.slice(1);
        const s = [];
        for (let j = 0; j + 4 <= hex.length; j += 4) {
          s.push(parseInt(hex.slice(j, j + 4), 16));
        }
        fetched.push(s);
      }
    }
    setTraces(fetched);
    setLoading(false);
  }, [sendCommand, traceIdx, count]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Trace Viewer</Text>
      <Text style={styles.hint}>Overlay captured traces to inspect alignment & leakage.</Text>

      <View style={styles.inputRow}>
        <Text style={styles.label}>Start idx:</Text>
        <TextInput style={styles.input} value={traceIdx} onChangeText={setTraceIdx} keyboardType="numeric" />
        <Text style={styles.label}>Count:</Text>
        <TextInput style={styles.input} value={count} onChangeText={setCount} keyboardType="numeric" />
      </View>

      <TouchableOpacity style={styles.fetchBtn} onPress={fetchTraces} disabled={loading}>
        <Text style={styles.fetchBtnText}>{loading ? 'Fetching...' : 'Fetch Traces'}</Text>
      </TouchableOpacity>

      <View style={styles.overlayBox}>
        {traces.map((s, idx) => (
          <View key={idx} style={styles.overlayLayer}>
            {s.map((v, i) => {
              const h = Math.abs((v - 0x8000) / 0xFFFF) * 110;
              return (
                <View key={i} style={[styles.bar, {
                  left: i * 1.3,
                  height: Math.max(1, h),
                  top: 60 - h / 2,
                  backgroundColor: COLORS[idx % COLORS.length],
                  opacity: 0.7,
                }]} />
              );
            })}
          </View>
        ))}
        {traces.length === 0 && <Text style={styles.placeholder}>No traces loaded</Text>}
      </View>

      <View style={styles.legend}>
        {traces.map((_, idx) => (
          <View key={idx} style={styles.legendItem}>
            <View style={[styles.legendDot, { backgroundColor: COLORS[idx % COLORS.length] }]} />
            <Text style={styles.legendText}>Trace {parseInt(traceIdx, 10) + idx}</Text>
          </View>
        ))}
      </View>

      <Text style={styles.tip}>Tip: well-aligned traces overlap tightly; leakage shows as
        a consistent spike at the same sample index across traces.</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35', marginBottom: 4 },
  hint: { color: '#8b949e', fontSize: 13, marginBottom: 16 },
  inputRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12, gap: 8 },
  label: { color: '#8b949e', fontSize: 13 },
  input: { backgroundColor: '#161b22', color: '#e6edf3', borderWidth: 1, borderColor: '#30363d', borderRadius: 6, padding: 8, width: 70, fontFamily: 'monospace' },
  fetchBtn: { backgroundColor: '#ff6b35', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  fetchBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  overlayBox: { backgroundColor: '#161b22', borderRadius: 10, height: 160, borderWidth: 1, borderColor: '#30363d', position: 'relative', padding: 8 },
  overlayLayer: { position: 'absolute', left: 8, right: 8, top: 10, bottom: 10 },
  bar: { position: 'absolute', width: 1.2 },
  placeholder: { color: '#6e7681', textAlign: 'center', marginTop: 60 },
  legend: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 12, gap: 12 },
  legendItem: { flexDirection: 'row', alignItems: 'center' },
  legendDot: { width: 10, height: 10, borderRadius: 5, marginRight: 6 },
  legendText: { color: '#8b949e', fontSize: 12 },
  tip: { color: '#6e7681', fontSize: 11, marginTop: 16, lineHeight: 16 },
});