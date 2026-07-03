/**
 * BandSniffScreen.js — victim-band auto-detect results
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Triggers the radar_cfg.c band-sniff routine and displays the detected
 * victim chirp parameters (start/stop frequency, bandwidth, chirp/PRI).
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ActivityIndicator } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function BandSniffScreen() {
  const { commands, bandInfo } = useDevice();
  const [sniffing, setSniffing] = useState(false);
  const [error, setError] = useState(null);

  const runSniff = async () => {
    setSniffing(true);
    setError(null);
    try {
      await commands.sniff(10);
      // bandInfo updates via DeviceContext notification
      setTimeout(() => setSniffing(false), 11000);
    } catch (e) {
      setError(String(e));
      setSniffing(false);
    }
  };

  const fmtHz = (hz) => {
    if (typeof hz !== 'bigint' && typeof hz !== 'number') return '—';
    const n = Number(hz);
    return `${(n / 1e9).toFixed(3)} GHz`;
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Band Sniff</Text>
      <Text style={styles.subtitle}>
        Passively sweeps the LO across 76–81 GHz to detect the victim radar's
        chirp parameters before injection.
      </Text>

      <Text style={styles.warn}>
        ⚠ Sniff is passive (RX only) but still requires RF rails. Ensure
        authorized operation in a shielded range.
      </Text>

      <TouchableOpacity style={styles.sniffBtn} onPress={runSniff} disabled={sniffing}>
        {sniffing ? (
          <View style={styles.row}>
            <ActivityIndicator color="#fff" />
            <Text style={styles.sniffBtnText}>  Sniffing (10s)…</Text>
          </View>
        ) : (
          <Text style={styles.sniffBtnText}>START SNIFF (10s)</Text>
        )}
      </TouchableOpacity>

      {error && <Text style={styles.error}>Error: {error}</Text>}

      <View style={styles.resultBox}>
        <Text style={styles.resultLabel}>DETECTED BAND</Text>
        <View style={styles.row}>
          <Text style={styles.resultKey}>Status</Text>
          <Text style={styles.resultVal}>
            {bandInfo ? (bandInfo.valid ? '✓ Valid' : '✗ No radar found') : '— not sniffed —'}
          </Text>
        </View>
        {bandInfo && bandInfo.valid && (
          <>
            <View style={styles.row}>
              <Text style={styles.resultKey}>Start freq</Text>
              <Text style={styles.resultVal}>{fmtHz(bandInfo.startHz)}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.resultKey}>Bandwidth</Text>
              <Text style={styles.resultVal}>{fmtHz(bandInfo.bwHz)}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.resultKey}>Stop freq (est)</Text>
              <Text style={styles.resultVal}>
                {bandInfo.startHz && bandInfo.bwHz
                  ? fmtHz(BigInt(bandInfo.startHz) + BigInt(bandInfo.bwHz))
                  : '—'}
              </Text>
            </View>
          </>
        )}
      </View>

      <Text style={styles.note}>
        The detected center frequency is used to set the DRFM sub-harmonic LO
        (RF_center / 2) for coherent injection.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 16 },
  title: { color: '#00ff9f', fontSize: 22, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 13, marginTop: 4, marginBottom: 16 },
  warn: { color: '#ffaa00', fontSize: 11, fontStyle: 'italic', marginBottom: 16 },
  sniffBtn: { backgroundColor: '#2a6e2a', padding: 16, borderRadius: 10, alignItems: 'center', marginBottom: 16 },
  sniffBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  row: { flexDirection: 'row', alignItems: 'center' },
  error: { color: '#ff5555', fontSize: 13, marginBottom: 12 },
  resultBox: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 16 },
  resultLabel: { color: '#666', fontSize: 11, fontWeight: 'bold', marginBottom: 12 },
  resultKey: { color: '#aaa', flex: 1, fontSize: 14 },
  resultVal: { color: '#00ff9f', flex: 1, fontSize: 14, fontWeight: 'bold' },
  note: { color: '#666', fontSize: 11, marginTop: 16, fontStyle: 'italic' },
});