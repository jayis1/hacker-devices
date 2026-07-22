// DashboardScreen.js — AxleTap device dashboard
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import StatusBar from '../components/StatusBar';
import * as Ble from '../utils/ble';
import * as Proto from '../utils/protocol';

export default function DashboardScreen() {
  const [connected, setConnected] = useState(false);
  const [armed, setArmed] = useState(false);
  const [captureRunning, setCaptureRunning] = useState(false);
  const [bleConnected, setBleConnected] = useState(false);
  const [status, setStatus] = useState(null);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    Ble.scanForDevices(
      (dev) => {
        // Auto-connect to first found
        Ble.connectToDevice(
          dev,
          (frame) => {
            // Status callback
            if (frame.type === Proto.MSG.STATUS) {
              const text = new TextDecoder().decode(frame.payload);
              setStatus(Proto.parseStatus(text));
            }
          },
          (frame) => { /* log */ }
        );
      },
      () => setScanning(false)
    );
    setScanning(true);
  }, []);

  const send = async (frame, label) => {
    const ok = await Ble.sendCommand(frame);
    if (!ok) Alert.alert('Error', `Failed to send: ${label}`);
  };

  return (
    <View style={styles.container}>
      <StatusBar
        connected={status?.phyA?.link || false}
        armed={armed}
        captureRunning={captureRunning}
        bleConnected={Ble.isConnected()}
      />

      <View style={styles.section}>
        <Text style={styles.h2}>Connection</Text>
        <Text style={styles.text}>
          {scanning ? 'Scanning for AxleTap…' : Ble.isConnected() ? 'Connected' : 'Not connected'}
        </Text>
        <TouchableOpacity
          style={styles.btn}
          onPress={() => {
            setScanning(true);
            Ble.scanForDevices(
              (d) => Ble.connectToDevice(d, (f) => setStatus(f), () => {}),
              () => setScanning(false)
            );
          }}
        >
          <Text style={styles.btnText}>Re-scan</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Links</Text>
        <View style={styles.row}>
          <View style={styles.card}>
            <Text style={styles.cardTitle}>PHY A (ECU)</Text>
            <Text style={styles.text}>Link: {status?.phyA?.link ? 'UP' : 'DOWN'}</Text>
            <Text style={styles.text}>Speed: {status?.phyA?.speed || '—'}</Text>
            <Text style={styles.text}>Role: {status?.phyA?.role || '—'}</Text>
          </View>
          <View style={styles.card}>
            <Text style={styles.cardTitle}>PHY B (Compute)</Text>
            <Text style={styles.text}>Link: {status?.phyB?.link ? 'UP' : 'DOWN'}</Text>
            <Text style={styles.text}>Speed: {status?.phyB?.speed || '—'}</Text>
            <Text style={styles.text}>Role: {status?.phyB?.role || '—'}</Text>
          </View>
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Arming</Text>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.btn, styles.armBtn]}
            onPress={() => { send(Proto.cmdArm(), 'arm'); setArmed(true); }}
          >
            <Text style={styles.btnText}>ARM</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.btn, styles.disarmBtn]}
            onPress={() => { send(Proto.cmdDisarm(), 'disarm'); setArmed(false); }}
          >
            <Text style={styles.btnText}>DISARM</Text>
          </TouchableOpacity>
        </View>
        {armed && (
          <Text style={styles.warn}>⚠ Transmit enabled — deploy only on authorized, stationary vehicle.</Text>
        )}
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Capture</Text>
        <View style={styles.row}>
          <TouchableOpacity
            style={styles.btn}
            onPress={() => { send(Proto.cmdPcapStart(), 'pcap start'); setCaptureRunning(true); }}
          >
            <Text style={styles.btnText}>Start Capture</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.btn}
            onPress={() => { send(Proto.cmdPcapStop(), 'pcap stop'); setCaptureRunning(false); }}
          >
            <Text style={styles.btnText}>Stop Capture</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.btn} onPress={() => send(Proto.cmdStatus(), 'status')}>
            <Text style={styles.btnText}>Refresh Status</Text>
          </TouchableOpacity>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212' },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#2a2a2a' },
  row: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 6 },
  h2: { color: '#7ab8ff', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  text: { color: '#ddd', fontSize: 12, fontFamily: 'monospace', marginVertical: 2 },
  card: {
    backgroundColor: '#1e1e1e', padding: 10, borderRadius: 6,
    marginRight: 10, marginBottom: 8, minWidth: 140, borderWidth: 1, borderColor: '#333',
  },
  cardTitle: { color: '#ffd166', fontSize: 12, fontWeight: 'bold', marginBottom: 6 },
  btn: {
    backgroundColor: '#2a4d6e', paddingHorizontal: 14, paddingVertical: 8,
    borderRadius: 4, marginRight: 8, marginBottom: 8,
  },
  armBtn: { backgroundColor: '#aa3333' },
  disarmBtn: { backgroundColor: '#33aa55' },
  btnText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
  warn: { color: '#ff6677', fontSize: 11, marginTop: 8, fontFamily: 'monospace' },
});