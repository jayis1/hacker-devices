/**
 * TDRReflectogramScreen.js — TDR acquisition + reflectogram plot
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Fires the TDR engine, receives the reflectogram in chunks over BLE,
 * and renders a live reflectogram plot. Displays the cable classification
 * result (type, impedance, length, shielded, live-conductor).
 */

import React, { useState, useRef, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { ReflectogramPlot } from '../utils/ReflectogramPlot';

const CABLE_TYPE_NAMES = [
  'Unknown', 'Cat 5e', 'Cat 6', 'Cat 6A', 'Cat 7',
  'Profibus DP', 'RS-485 Belden 9841', 'MIL-1553 Twinax',
  'POTS Riser', 'Power/Mains (REFUSE)',
];

export default function TDRReflectogramScreen() {
  const { fireTDR, setTdrCallback, MODE, enterMode } = useDevice();
  const [result, setResult] = useState(null);
  const [reflectogram, setReflectogram] = useState([]);
  const [firing, setFiring] = useState(false);

  const resultRef = useRef(null);

  const handleTdrData = useCallback((data, kind) => {
    if (kind === 'header') {
      resultRef.current = { ...data, reflectogram: [] };
      setResult(resultRef.current);
      setReflectogram([]);
    } else if (kind === 'chunk') {
      // Append the chunk's samples at the right offset
      setReflectogram((prev) => {
        const next = [...prev];
        for (let i = 0; i < data.samples.length; i++) {
          next[data.offset + i] = data.samples[i];
        }
        return next;
      });
    }
  }, []);

  const handleFire = async () => {
    setFiring(true);
    setResult(null);
    setReflectogram([]);
    setTdrCallback(handleTdrData);
    await enterMode(MODE.TDR);
    await fireTDR();
    // The reflectogram arrives via TDR_RESULT + TDR_CHUNK notifications
    setTimeout(() => setFiring(false), 5000);
  };

  const impedanceOhms = result ? Math.round(result.impedance_mohm / 1000) : 0;
  const lengthM = result ? (result.length_mm / 1000).toFixed(2) : '0.00';

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>TDR Reflectogram</Text>
      <Text style={styles.desc}>
        Clamp the jaws around a cable, then fire the TDR to fingerprint it.
        The reflectogram shows reflections vs. distance.
      </Text>

      <TouchableOpacity style={styles.fireButton} onPress={handleFire} disabled={firing}>
        <Text style={styles.fireButtonText}>{firing ? 'Firing...' : '🔥 Fire TDR'}</Text>
      </TouchableOpacity>

      {result && (
        <View style={styles.resultCard}>
          <Text style={styles.resultTitle}>Classification</Text>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Cable type:</Text>
            <Text style={[styles.resultValue, { color: result.type === 9 ? '#f85149' : '#00d4aa' }]}>
              {CABLE_TYPE_NAMES[result.type] || 'Unknown'}
            </Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Impedance:</Text>
            <Text style={styles.resultValue}>{impedanceOhms} Ω</Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Length:</Text>
            <Text style={styles.resultValue}>{lengthM} m</Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Shielded:</Text>
            <Text style={styles.resultValue}>{result.shielded ? 'Yes (coupling weak)' : 'No'}</Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Live conductor:</Text>
            <Text style={[styles.resultValue, { color: result.live_conductor ? '#f85149' : '#00d4aa' }]}>
              {result.live_conductor ? 'YES — DO NOT INJECT' : 'No (safe for inject)'}
            </Text>
          </View>
        </View>
      )}

      <View style={styles.plotContainer}>
        <Text style={styles.plotTitle}>Reflectogram</Text>
        <ReflectogramPlot data={reflectogram} />
      </View>

      <Text style={styles.hint}>
        Tap "Fire TDR" again to re-acquire. Use the classification to decide
        whether to enter Sniff or Inject mode.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0d1117' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35', marginBottom: 6 },
  desc: { color: '#8b949e', fontSize: 13, marginBottom: 16 },
  fireButton: { backgroundColor: '#ff6b35', padding: 16, borderRadius: 10, alignItems: 'center', marginBottom: 16 },
  fireButtonText: { color: '#fff', fontSize: 18, fontWeight: 'bold' },
  resultCard: { backgroundColor: '#161b22', padding: 16, borderRadius: 10, borderWidth: 1, borderColor: '#30363d', marginBottom: 16 },
  resultTitle: { color: '#f0f6fc', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  resultRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  resultLabel: { color: '#8b949e', fontSize: 14 },
  resultValue: { color: '#f0f6fc', fontSize: 14, fontWeight: '600' },
  plotContainer: { backgroundColor: '#161b22', padding: 12, borderRadius: 10, marginBottom: 16, minHeight: 180 },
  plotTitle: { color: '#8b949e', fontSize: 12, marginBottom: 8 },
  hint: { color: '#6e7681', fontSize: 12, textAlign: 'center', marginBottom: 20 },
});