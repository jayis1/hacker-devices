/**
 * DashboardScreen.js — Live status overview
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import VBUSGauge from '../components/VBUSGauge';
import { CMD, MODE_NAMES, EmberTapConnection } from '../utils/protocol';

let connection = null;

export default function DashboardScreen() {
  const [status, setStatus] = useState({
    mode: 0, vbus_mv: 0, vbus_ma: 0, temp_c: 0,
    fuzz_sent: 0, fuzz_crash: 0,
  });
  const [connected, setConnected] = useState(false);

  const handlePacket = useCallback((pkt) => {
    if (pkt.cmd === 0x82) {
      const { decodeStatus } = require('../utils/protocol');
      const s = decodeStatus(pkt.payload);
      if (s) setStatus(s);
    }
  }, []);

  const doConnect = async () => {
    try {
      /* In production this opens the USB-OTG serial or BT SPP transport.
       * Here we stub the transport for UI demonstration. */
      const transport = {
        opened: false,
        cb: null,
        async open() { this.opened = true; },
        async close() { this.opened = false; },
        async write(buf) { /* send to device */ },
        onData(cb) { this.cb = cb; },
      };
      connection = new EmberTapConnection();
      connection.onPacket = handlePacket;
      await connection.connect(transport);
      setConnected(true);
      /* Request initial status */
      await connection.send(CMD.STATUS);
    } catch (e) {
      Alert.alert('Connection failed', String(e));
    }
  };

  useEffect(() => {
    if (!connected) return;
    const interval = setInterval(() => {
      if (connection) connection.send(CMD.STATUS);
    }, 1000);
    return () => clearInterval(interval);
  }, [connected]);

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Ember-Tap</Text>
        <Text style={styles.subtitle}>by jayis1</Text>
      </View>

      <TouchableOpacity
        style={[styles.connectBtn, connected && styles.connectedBtn]}
        onPress={doConnect}
      >
        <Text style={styles.connectText}>
          {connected ? '● Connected' : '○ Connect'}
        </Text>
      </TouchableOpacity>

      <VBUSGauge mv={status.vbus_mv} ma={status.vbus_ma} />

      <View style={styles.modeRow}>
        <Text style={styles.label}>Mode:</Text>
        <Text style={styles.modeValue}>
          {MODE_NAMES[status.mode] || 'UNKNOWN'}
        </Text>
      </View>

      <View style={styles.statGrid}>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Temperature</Text>
          <Text style={styles.statValue}>{status.temp_c}°C</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Fuzz Sent</Text>
          <Text style={styles.statValue}>{status.fuzz_sent}</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Crashes</Text>
          <Text style={[styles.statValue, styles.crashValue]}>
            {status.fuzz_crash}
          </Text>
        </View>
      </View>

      <Text style={styles.disclaimer}>
        ⚠ Authorized security research only. Misuse may damage hardware.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  title: { color: '#FF6600', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 14, alignSelf: 'flex-end' },
  connectBtn: {
    backgroundColor: '#16213e', padding: 12, borderRadius: 8,
    alignItems: 'center', marginBottom: 16,
    borderWidth: 1, borderColor: '#FF6600',
  },
  connectedBtn: { backgroundColor: '#1a3a1a', borderColor: '#4CAF50' },
  connectText: { color: '#fff', fontSize: 16, fontWeight: '600' },
  modeRow: { flexDirection: 'row', marginBottom: 12 },
  label: { color: '#888', fontSize: 16 },
  modeValue: { color: '#FF6600', fontSize: 16, fontWeight: 'bold', marginLeft: 8 },
  statGrid: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 16 },
  statCard: {
    backgroundColor: '#16213e', borderRadius: 8, padding: 12,
    flex: 1, marginHorizontal: 4, alignItems: 'center',
  },
  statLabel: { color: '#888', fontSize: 11, marginBottom: 4 },
  statValue: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  crashValue: { color: '#FF4444' },
  disclaimer: {
    color: '#FFAA00', fontSize: 11, marginTop: 20, textAlign: 'center',
    fontStyle: 'italic',
  },
});

/* end of file — author: jayis1 */