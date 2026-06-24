/*
 * lora-phantom / app / screens / SpectrumScanScreen.js
 * Channel activity scan with RSSI heatmap and DR distribution.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, buildScanPayload, REGIONS, REGION_NAMES, parseScanResult } from '../utils/protocol';

export default function SpectrumScanScreen() {
  const { connected, status, scanResults, sendCommand } = useDevice();
  const [dwell, setDwell] = useState('500');
  const [scanning, setScanning] = useState(false);

  const startScan = () => {
    const dwellMs = parseInt(dwell, 10);
    sendCommand(CMD.SCAN_START, buildScanPayload(dwellMs));
    setScanning(true);
    setTimeout(() => setScanning(false), dwellMs * 8 + 2000);
  };

  // Find max RSSI for bar scaling
  const maxRssi = scanResults.length > 0
    ? Math.max(...scanResults.map(r => Math.abs(r.rssi)))
    : 100;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>📊 Spectrum Scanner</Text>
      <Text style={styles.description}>
        Scans regional channels for LoRa activity. Maps RSSI per channel and
        detects active transmissions via CAD (Channel Activity Detection).
      </Text>

      {status && (
        <View style={styles.regionBox}>
          <Text style={styles.regionText}>Region: {REGION_NAMES[status.region]}</Text>
          <Text style={styles.regionDetail}>Scanning {REGIONS[status.region]} channels</Text>
        </View>
      )}

      <View style={styles.row}>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Dwell per channel (ms)</Text>
          <TextInput style={styles.input} value={dwell} onChangeText={setDwell}
            keyboardType="number-pad" editable={!scanning} />
        </View>
        <TouchableOpacity style={styles.scanBtn} onPress={startScan} disabled={scanning || !connected}>
          <Text style={styles.btnText}>{scanning ? 'Scanning...' : '▶ Scan'}</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.section}>Channel Results ({scanResults.length})</Text>

      {scanResults.length === 0 ? (
        <Text style={styles.empty}>No scan results. Run a scan to see channel activity.</Text>
      ) : (
        <View style={styles.resultsContainer}>
          {scanResults.map((r, i) => {
            const freqMHz = (r.freqHz / 1e6).toFixed(1);
            const rssiAbs = Math.abs(r.rssi);
            const barWidth = Math.max(5, (rssiAbs / maxRssi) * 100);
            return (
              <View key={i} style={styles.channelRow}>
                <Text style={styles.channelFreq}>{freqMHz} MHz</Text>
                <View style={styles.barContainer}>
                  <View style={[
                    styles.bar,
                    { width: barWidth + '%' },
                    r.activity ? styles.barActive : styles.barSilent,
                  ]} />
                </View>
                <Text style={styles.channelRssi}>{r.rssi}dBm</Text>
                {r.activity && <Text style={styles.activityDot}>●</Text>}
                <Text style={styles.hits}>{r.hits} hits</Text>
              </View>
            );
          })}
        </View>
      )}

      {scanResults.length > 0 && (
        <View style={styles.summaryBox}>
          <Text style={styles.summaryTitle}>Scan Summary</Text>
          <Text style={styles.summaryText}>
            Active channels: {scanResults.filter(r => r.activity).length} / {scanResults.length}
          </Text>
          <Text style={styles.summaryText}>
            Avg RSSI: {Math.round(scanResults.reduce((s, r) => s + r.rssi, 0) / scanResults.length)} dBm
          </Text>
          <Text style={styles.summaryText}>
            Total hits: {scanResults.reduce((s, r) => s + r.hits, 0)}
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 8 },
  description: { color: '#888', fontSize: 12, marginBottom: 12, lineHeight: 18 },
  regionBox: { backgroundColor: '#1a1a1a', borderRadius: 6, padding: 10, marginBottom: 10,
               borderWidth: 1, borderColor: '#333' },
  regionText: { color: '#00ff88', fontSize: 14, fontWeight: 'bold' },
  regionDetail: { color: '#888', fontSize: 11, marginTop: 2 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 10, alignItems: 'flex-end' },
  configItem: { flex: 1 },
  configLabel: { color: '#888', fontSize: 11, marginBottom: 3 },
  input: { backgroundColor: '#1a1a1a', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 8, fontSize: 14 },
  scanBtn: { backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88',
             borderRadius: 6, padding: 12, paddingHorizontal: 20 },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: 'bold' },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 15, marginBottom: 8 },
  empty: { color: '#888', fontSize: 13, fontStyle: 'italic' },
  resultsContainer: { marginBottom: 10 },
  channelRow: { flexDirection: 'row', alignItems: 'center', gap: 6, marginBottom: 4 },
  channelFreq: { color: '#aaa', fontSize: 11, width: 75, fontFamily: 'monospace' },
  barContainer: { flex: 1, height: 16, backgroundColor: '#1a1a1a', borderRadius: 3, overflow: 'hidden' },
  bar: { height: '100%', borderRadius: 3 },
  barActive: { backgroundColor: '#00ff88' },
  barSilent: { backgroundColor: '#444' },
  channelRssi: { color: '#888', fontSize: 10, width: 50, fontFamily: 'monospace' },
  activityDot: { color: '#00ff88', fontSize: 12 },
  hits: { color: '#666', fontSize: 10, width: 50 },
  summaryBox: { backgroundColor: '#1a1a1a', borderRadius: 6, padding: 12, marginTop: 10,
                borderWidth: 1, borderColor: '#00ff88' },
  summaryTitle: { color: '#00ff88', fontSize: 14, fontWeight: 'bold', marginBottom: 5 },
  summaryText: { color: '#ccc', fontSize: 12, marginBottom: 2 },
});