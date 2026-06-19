// screens/DashboardScreen.js — live status overview
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import { MSG } from '../utils/protocol';

export default function DashboardScreen() {
  const { status, connected, connect, send } = useReaper();

  const doConnect = () => {
    Alert.alert('Connect', 'Scanning BLE for Powerline-Reaper...');
    connect();
  };

  const doZeroize = () => {
    Alert.alert('Zeroize', 'Wipe all keys and SD?', [
      { text: 'Cancel' },
      { text: 'Zeroize', style: 'destructive', onPress: () => send(MSG.ZEROIZE) },
    ]);
  };

  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.section}>
        <Text style={styles.h1}>Powerline-Reaper</Text>
        <Text style={styles.sub}>PLC interception & injection tool</Text>
        <Text style={styles.author}>author: jayis1 · MIT</Text>
      </View>

      {!connected ? (
        <TouchableOpacity style={styles.btn} onPress={doConnect}>
          <Text style={styles.btnText}>Connect via BLE</Text>
        </TouchableOpacity>
      ) : (
        <View style={styles.section}>
          <Row label="PLC Link"   value={status.plcLink ? 'UP' : 'DOWN'} color={status.plcLink ? '#00ffaa' : '#ff5555'} />
          <Row label="Mode"       value={status.mode} />
          <Row label="Network ID" value={status.networkId || '—'} />
          <Row label="CCo MAC"    value={status.ccoMac || '—'} />
          <Row label="Stations"   value={String(status.stationCount)} />
          <Row label="Battery"   value={`${status.batteryPct}% (${status.batteryMv} mV)`} />
          <Row label="SD Free"    value={`${(status.sdFree / 1024).toFixed(1)} MB`} />
          <Row label="Cap Rate"   value={`${status.captureRate} fps`} />
          <Row label="Caps flags" value={`0x${status.caps.toString(16)}`} />
        </View>
      )}

      <TouchableOpacity style={[styles.btn, styles.btnDanger]} onPress={doZeroize}>
        <Text style={styles.btnText}>Zeroize Keys + SD</Text>
      </TouchableOpacity>
    </View>
  );
}

const Row = ({ label, value, color = '#eee' }) => (
  <View style={styles.row}>
    <Text style={styles.rowLabel}>{label}</Text>
    <Text style={[styles.rowVal, { color }]}>{value}</Text>
  </View>
);

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  section: { marginTop: 12, marginBottom: 12 },
  h1: { color: '#eee', fontSize: 22, fontWeight: 'bold' },
  sub: { color: '#888', fontSize: 13 },
  author: { color: '#555', fontSize: 11, marginTop: 4 },
  btn: { backgroundColor: '#1a3a1a', padding: 14, borderRadius: 6, marginTop: 12, alignItems: 'center' },
  btnDanger: { backgroundColor: '#3a1a1a' },
  btnText: { color: '#00ffaa', fontSize: 15, fontWeight: 'bold' },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8, borderBottomWidth: 0.5, borderBottomColor: '#222' },
  rowLabel: { color: '#888', fontSize: 13, fontFamily: 'monospace' },
  rowVal: { color: '#eee', fontSize: 13, fontFamily: 'monospace' },
});