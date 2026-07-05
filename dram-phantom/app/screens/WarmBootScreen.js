/**
 * WarmBootScreen.js — warm-boot memory acquisition control
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Configures a region of DRAM to acquire after host reset, arms the device,
 * triggers acquisition, and shows progress + resulting image download.
 */

import React, { useState, useCallback } from 'react';
import { View, Text, StyleSheet, Button, TextInput, Alert, ScrollView, ProgressBarAndroid } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function WarmBootScreen() {
  const { connected, status, setMode, arm, warmBoot } = useDevice();
  const [baseRow, setBaseRow] = useState('0');
  const [rowCount, setRowCount] = useState('32768'); // 256 MB at 8KB/row
  const [token, setToken] = useState('');
  const [progress, setProgress] = useState(0);
  const [running, setRunning] = useState(false);
  const [imageSha, setImageSha] = useState('');

  const doAcquire = useCallback(async () => {
    try {
      await setMode(3); // MODE_WARMBOOT
      if (token.length !== 6) {
        Alert.alert('Bad token', 'Enter the 6-char token from the OLED.');
        return;
      }
      await arm(token);
      setRunning(true);
      setProgress(0);
      // Poll progress locally; the device sends "OK WARMBOOT done" when finished.
      const interval = setInterval(() => {
        setProgress((p) => Math.min(p + 0.05, 0.95));
      }, 1000);
      const r = await warmBoot(parseInt(baseRow, 10), parseInt(rowCount, 10));
      clearInterval(interval);
      setProgress(1);
      if (r.startsWith('OK')) {
        Alert.alert('Acquisition complete', r);
        setImageSha('sha256: (computed on device, see SD card)');
      } else {
        Alert.alert('Acquisition failed', r);
      }
    } catch (e) {
      Alert.alert('Error', e.message);
    } finally {
      setRunning(false);
    }
  }, [setMode, arm, warmBoot, baseRow, rowCount, token]);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Warm-Boot Acquisition</Text>
        <Text style={styles.status}>{connected ? 'connected' : 'disconnected'}</Text>
      </View>

      <View style={styles.warn}>
        <Text style={styles.warnText}>
          ⚠️ On host reset the FPGA holds DRAM refresh from battery and drains
          the configured region to SD / USB. Battery budget ~90 s — keep host
          VDD alive for full-module capture. Authorized use only. Author: jayis1.
        </Text>
      </View>

      <Text style={styles.label}>Base row (decimal)</Text>
      <TextInput style={styles.input} keyboardType="numeric" value={baseRow} onChangeText={setBaseRow} />

      <Text style={styles.label}>Row count (8 KB/row → 32768 ≈ 256 MB)</Text>
      <TextInput style={styles.input} keyboardType="numeric" value={rowCount} onChangeText={setRowCount} />

      <Text style={styles.label}>Arm token (from device OLED)</Text>
      <TextInput style={styles.input} value={token} onChangeText={setToken}
        maxLength={6} placeholder="6-char token" autoCapitalize="characters" />

      <View style={styles.controls}>
        <Button title={running ? 'Acquiring...' : 'Acquire'} color="#d29922"
          onPress={doAcquire} disabled={running} />
      </View>

      {running || progress > 0 ? (
        <View style={styles.progressBox}>
          <ProgressBarAndroid styleAttr="Horizontal" indeterminate={false} progress={progress} />
          <Text style={styles.progressText}>{Math.round(progress * 100)}%</Text>
        </View>
      ) : null}

      {imageSha ? (
        <View style={styles.resultBox}>
          <Text style={styles.resultLabel}>Result</Text>
          <Text style={styles.resultText}>{imageSha}</Text>
          <Text style={styles.resultText}>Image written to microSD: dump_*.bin</Text>
          <Text style={styles.resultText}>Analyse with Volatility/rekall offline.</Text>
        </View>
      ) : null}

      <Text style={styles.footer}>Author: jayis1 · DRAM-Phantom v1.0</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 12 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  title: { color: '#e6edf3', fontSize: 18, fontWeight: 'bold' },
  status: { color: '#7d8590', fontSize: 12 },
  warn: { backgroundColor: '#3d2f1f', padding: 8, borderRadius: 4, marginBottom: 12, borderLeftWidth: 3, borderLeftColor: '#d29922' },
  warnText: { color: '#ffb870', fontSize: 11 },
  label: { color: '#7d8590', fontSize: 12, marginTop: 8, marginBottom: 4 },
  input: { backgroundColor: '#161b22', color: '#e6edf3', borderRadius: 4, padding: 8, fontSize: 14, fontFamily: 'monospace' },
  controls: { flexDirection: 'row', justifyContent: 'center', marginTop: 12, marginBottom: 12 },
  progressBox: { marginVertical: 12 },
  progressText: { color: '#58a6ff', textAlign: 'center', fontSize: 12, marginTop: 4 },
  resultBox: { backgroundColor: '#161b22', padding: 12, borderRadius: 4, marginTop: 8 },
  resultLabel: { color: '#3fb950', fontSize: 13, fontWeight: 'bold', marginBottom: 4 },
  resultText: { color: '#e6edf3', fontSize: 12, fontFamily: 'monospace', marginBottom: 2 },
  footer: { color: '#484f58', fontSize: 10, textAlign: 'center', marginTop: 16 },
});