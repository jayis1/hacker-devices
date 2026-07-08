/*
 * tpm-phantom — app/screens/DashboardScreen.js
 * Main dashboard showing device status, capture controls, and stats.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import {
  CMD_GET_STATUS, CMD_START_CAP, CMD_STOP_CAP, CMD_GET_STATS,
  CMD_GET_VERSION, CMD_RESET, MODE_LPC, MODE_SPI, MODE_IDLE,
  RSP_STATUS, RSP_STATS, RSP_VERSION, decodeStatus, decodeStats,
  decodeVersion,
} from '../utils/protocol';

export default function DashboardScreen({ ble, status, stats, onStatusUpdate, onStatsUpdate }) {
  const [version, setVersion] = useState('');
  const [busy, setBusy] = useState(false);

  // Periodically poll status
  useEffect(() => {
    const interval = setInterval(() => {
      if (ble.isConnected()) {
        ble.send(CMD_GET_STATUS);
        ble.send(CMD_GET_STATS);
      }
    }, 2000);
    return () => clearInterval(interval);
  }, [ble]);

  const handleStart = useCallback((mode) => {
    setBusy(true);
    ble.send(CMD_START_CAP, new Uint8Array([mode, 0])).then(() => {
      onStatusUpdate({ ...status, capturing: true, mode });
      setBusy(false);
    });
  }, [ble, status, onStatusUpdate]);

  const handleStop = useCallback(() => {
    ble.send(CMD_STOP_CAP);
    onStatusUpdate({ ...status, capturing: false });
  }, [ble, status, onStatusUpdate]);

  const handleReset = useCallback(() => {
    Alert.alert('Reset Stats', 'Clear all capture statistics?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Reset', onPress: () => ble.send(CMD_RESET), style: 'destructive' },
    ]);
  }, [ble]);

  const handleGetVersion = useCallback(async () => {
    await ble.send(CMD_GET_VERSION);
  }, [ble]);

  const modeText = status.mode === MODE_LPC ? 'LPC' : status.mode === MODE_SPI ? 'SPI' : 'IDLE';

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Status</Text>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Connection:</Text>
          <Text style={[styles.value, ble.isConnected() ? styles.green : styles.red]}>
            {ble.isConnected() ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Mode:</Text>
          <Text style={styles.value}>{modeText}</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Capturing:</Text>
          <Text style={[styles.value, status.capturing ? styles.green : styles.gray]}>
            {status.capturing ? 'ACTIVE' : 'IDLE'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>SD Card:</Text>
          <Text style={[styles.value, status.sdReady ? styles.green : styles.red]}>
            {status.sdReady ? 'Ready' : 'Not Present'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>USB:</Text>
          <Text style={[styles.value, status.usbConnected ? styles.green : styles.gray]}>
            {status.usbConnected ? 'Connected' : 'N/A'}
          </Text>
        </View>
        <TouchableOpacity style={styles.smallBtn} onPress={handleGetVersion}>
          <Text style={styles.btnText}>Get Firmware Version</Text>
        </TouchableOpacity>
        {version ? <Text style={styles.versionText}>{version}</Text> : null}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Capture Control</Text>
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.button, styles.startBtn]}
            disabled={busy || status.capturing}
            onPress={() => handleStart(MODE_LPC)}>
            <Text style={styles.btnText}>Start LPC</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.startBtn]}
            disabled={busy || status.capturing}
            onPress={() => handleStart(MODE_SPI)}>
            <Text style={styles.btnText}>Start SPI</Text>
          </TouchableOpacity>
        </View>
        <TouchableOpacity
          style={[styles.button, styles.stopBtn]}
          disabled={!status.capturing}
          onPress={handleStop}>
          <Text style={styles.btnText}>Stop Capture</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.button, styles.resetBtn]}
          onPress={handleReset}>
          <Text style={styles.btnText}>Reset Stats</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Capture Statistics</Text>
        <View style={styles.statsGrid}>
          <StatBlock label="LPC Total" value={stats.lpcTotal} color="#ffaa00" />
          <StatBlock label="LPC TPM" value={stats.lpcTpm} color="#ff6600" />
          <StatBlock label="SPI Total" value={stats.spiTotal} color="#00aaff" />
          <StatBlock label="SPI Reads" value={stats.spiReads} color="#00ff88" />
          <StatBlock label="SPI Writes" value={stats.spiWrites} color="#ff6644" />
          <StatBlock label="SPI Bytes" value={stats.spiBytes} color="#ffdd44" />
          <StatBlock label="SD Blocks" value={stats.sdBlocks} color="#aa88ff" />
          <StatBlock label="SD Bytes" value={stats.sdBytes} color="#88aaff" />
          <StatBlock label="SERIRQ" value={stats.lpcSerirq} color="#ff88ff" />
        </View>
      </View>

      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠ tpm-phantom is for authorized security research only.
          Unauthorized interception of TPM traffic may violate law.
          Author: jayis1 — GPL-2.0 / CERN-OHL-S v2
        </Text>
      </View>
    </ScrollView>
  );
}

function StatBlock({ label, value, color }) {
  return (
    <View style={styles.statBlock}>
      <Text style={[styles.statValue, { color }]}>{value || 0}</Text>
      <Text style={styles.statLabel}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d1a' },
  card: {
    backgroundColor: '#1a1a2e',
    margin: 8,
    padding: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  cardTitle: {
    color: '#8888ff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 10,
    fontFamily: 'monospace',
  },
  statusRow: { flexDirection: 'row', marginBottom: 6 },
  label: { color: '#888', fontSize: 13, width: 100, fontFamily: 'monospace' },
  value: { color: '#e0e0e0', fontSize: 13, fontFamily: 'monospace', fontWeight: 'bold' },
  green: { color: '#00ff88' },
  red: { color: '#ff3344' },
  gray: { color: '#666' },
  buttonRow: { flexDirection: 'row', marginBottom: 8 },
  button: {
    flex: 1,
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  startBtn: { backgroundColor: '#0a5' },
  stopBtn: { backgroundColor: '#a33', marginTop: 4 },
  resetBtn: { backgroundColor: '#444', marginTop: 4 },
  btnText: { color: '#fff', fontSize: 14, fontFamily: 'monospace', fontWeight: 'bold' },
  smallBtn: {
    marginTop: 8,
    padding: 6,
    backgroundColor: '#333',
    borderRadius: 4,
    alignItems: 'center',
  },
  versionText: {
    color: '#aaaaff',
    fontSize: 11,
    fontFamily: 'monospace',
    marginTop: 4,
    textAlign: 'center',
  },
  statsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  statBlock: {
    width: '32%',
    backgroundColor: '#111120',
    padding: 8,
    marginBottom: 6,
    borderRadius: 4,
    alignItems: 'center',
  },
  statValue: {
    fontSize: 18,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  statLabel: {
    color: '#888',
    fontSize: 10,
    fontFamily: 'monospace',
    marginTop: 2,
  },
  disclaimer: {
    margin: 8,
    padding: 10,
    backgroundColor: '#221111',
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#ff3344',
  },
  disclaimerText: {
    color: '#ff6666',
    fontSize: 10,
    fontFamily: 'monospace',
    textAlign: 'center',
  },
});