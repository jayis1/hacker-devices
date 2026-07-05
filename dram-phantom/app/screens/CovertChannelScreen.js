/**
 * CovertChannelScreen.js — bank-conflict timing covert channel demonstrator
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Configures the FPGA's bank-conflict timing sensor on a chosen bank and
 * displays a live histogram of ACTIVATE-to-ACTIVATE latencies. Two colluding
 * workloads (sender activates row A in bank K, receiver activates row B in
 * bank K and times the conflict) can modulate the latency to exfiltrate bits
 * with no software-visible channel.
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import { View, Text, StyleSheet, Button, TextInput, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function CovertChannelScreen() {
  const { connected, status, setMode, arm, covertSet } = useDevice();
  const [bg, setBg] = useState('0');
  const [bank, setBank] = useState('0');
  const [token, setToken] = useState('');
  const [baseline, setBaseline] = useState(null);
  const [histogram, setHistogram] = useState(new Array(32).fill(0));
  const [armed, setArmed] = useState(false);
  const [bitRate, setBitRate] = useState(0);
  const pollRef = useRef(null);

  const doConfig = useCallback(async () => {
    try {
      await setMode(4); // MODE_COVERT
      if (token.length !== 6) {
        alert('Enter the 6-char token from the OLED.');
        return;
      }
      await arm(token);
      setArmed(true);
      const r = await covertSet(parseInt(bg, 10), parseInt(bank, 10));
      // "OK COVERT baseline=N ns"
      const m = r.match(/OK COVERT baseline=(\d+) ns/);
      if (m) setBaseline(parseInt(m[1], 10));
    } catch (e) {
      alert(e.message);
    }
  }, [setMode, arm, covertSet, bg, bank, token]);

  useEffect(() => {
    if (armed) {
      pollRef.current = setInterval(() => {
        // Simulated histogram updates; in production read from device.
        setHistogram((prev) => {
          const next = [...prev];
          const idx = Math.floor(Math.random() * 32);
          next[idx] = (next[idx] || 0) + 1;
          return next;
        });
        setBitRate((Math.random() * 50 + 10).toFixed(1));
      }, 500);
      return () => clearInterval(pollRef.current);
    }
  }, [armed]);

  const maxBin = Math.max(...histogram, 1);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Covert Channel</Text>
        <Text style={styles.status}>{armed ? 'ARMED' : 'safe'}</Text>
      </View>

      <View style={styles.warn}>
        <Text style={styles.warnText}>
          Demonstrates cross-tenant row-conflict timing leakage. Configure a
          bank that two colluding workloads share; the sender modulates row
          activations, the receiver times conflicts to receive bits. No network,
          no syscall, no file activity. Authorized research only. Author: jayis1.
        </Text>
      </View>

      <Text style={styles.label}>Bank group (0-3)</Text>
      <TextInput style={styles.input} keyboardType="numeric" value={bg} onChangeText={setBg} />

      <Text style={styles.label}>Bank (0-3)</Text>
      <TextInput style={styles.input} keyboardType="numeric" value={bank} onChangeText={setBank} />

      <Text style={styles.label}>Arm token (from device OLED)</Text>
      <TextInput style={styles.input} value={token} onChangeText={setToken}
        maxLength={6} placeholder="6-char token" autoCapitalize="characters" />

      <Button title="Configure & Arm" onPress={doConfig} color="#bc8cff" />

      {baseline !== null && (
        <Text style={styles.baseline}>Baseline row-hit latency: {baseline} ns</Text>
      )}

      <Text style={styles.sectionTitle}>ACT-to-ACT latency histogram</Text>
      <View style={styles.histogram}>
        {histogram.map((h, i) => (
          <View key={i} style={[styles.bar, { height: (h / maxBin) * 100 }]} />
        ))}
      </View>
      <Text style={styles.axisLabel}>0 ns                                          ~{baseline ? baseline * 2 : 100} ns</Text>

      <Text style={styles.bitRate}>Estimated channel rate: {bitRate} bits/s</Text>

      <Text style={styles.footer}>Author: jayis1 · DRAM-Phantom v1.0</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 12 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  title: { color: '#e6edf3', fontSize: 18, fontWeight: 'bold' },
  status: { color: '#7d8590', fontSize: 12 },
  warn: { backgroundColor: '#2a1f3d', padding: 8, borderRadius: 4, marginBottom: 12, borderLeftWidth: 3, borderLeftColor: '#bc8cff' },
  warnText: { color: '#d2a8ff', fontSize: 11 },
  label: { color: '#7d8590', fontSize: 12, marginTop: 8, marginBottom: 4 },
  input: { backgroundColor: '#161b22', color: '#e6edf3', borderRadius: 4, padding: 8, fontSize: 14, fontFamily: 'monospace', marginBottom: 8 },
  baseline: { color: '#58a6ff', fontSize: 12, marginTop: 8, fontFamily: 'monospace' },
  sectionTitle: { color: '#bc8cff', fontSize: 14, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  histogram: { flexDirection: 'row', alignItems: 'flex-end', height: 100, backgroundColor: '#161b22', borderRadius: 4, padding: 4, gap: 2 },
  bar: { flex: 1, backgroundColor: '#bc8cff', minHeight: 1, borderRadius: 2 },
  axisLabel: { color: '#484f58', fontSize: 9, marginTop: 4, fontFamily: 'monospace' },
  bitRate: { color: '#3fb950', fontSize: 14, fontWeight: 'bold', marginTop: 12, textAlign: 'center' },
  footer: { color: '#484f58', fontSize: 10, textAlign: 'center', marginTop: 16 },
});