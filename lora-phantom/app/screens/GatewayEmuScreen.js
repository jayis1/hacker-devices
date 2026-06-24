/*
 * lora-phantom / app / screens / GatewayEmuScreen.js
 * Rogue gateway / network server emulation control.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, hexToBytes } from '../utils/protocol';

export default function GatewayEmuScreen() {
  const { connected, status, sendCommand, log } = useDevice();
  const [nwkKey, setNwkKey] = useState('00000000000000000000000000000000');
  const [active, setActive] = useState(false);
  const [sessions, setSessions] = useState([]);

  const start = () => {
    if (nwkKey.length !== 32) return;
    const keyBytes = hexToBytes(nwkKey);
    sendCommand(CMD.GWEMU_START, keyBytes);
    setActive(true);
  };

  const stop = () => {
    sendCommand(CMD.GWEMU_STOP);
    setActive(false);
  };

  // Filter log for rogue session entries
  const sessionLogs = log.filter(l => l.msg.includes('Rogue session'));

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>🏠 Rogue Gateway Emulator</Text>
      <Text style={styles.description}>
        Emulates a LoRaWAN network server to answer join-requests from end-devices
        with forged join-accepts. Establishes rogue sessions for device control.
      </Text>

      <Text style={styles.section}>JoinKey (NwkKey)</Text>
      <TextInput
        style={styles.keyInput}
        value={nwkKey}
        onChangeText={setNwkKey}
        maxLength={32}
        placeholder="NwkKey (32 hex chars)"
        placeholderTextColor="#555"
        editable={!active}
      />

      <View style={styles.btnRow}>
        {!active ? (
          <TouchableOpacity style={styles.startBtn} onPress={start} disabled={!connected || nwkKey.length !== 32}>
            <Text style={styles.btnText}>▶ Start Gateway Emulation</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.stopBtn} onPress={stop}>
            <Text style={styles.btnText}>⏹ Stop</Text>
          </TouchableOpacity>
        )}
      </View>

      {active && (
        <View style={styles.activeBox}>
          <Text style={styles.activeText}>🟡 Listening for join-requests...</Text>
          <Text style={styles.activeHint}>Device will answer join-requests on RX1 window (5s delay).</Text>
        </View>
      )}

      <Text style={styles.section}>Rogue Sessions ({sessionLogs.length})</Text>
      {sessionLogs.length === 0 ? (
        <Text style={styles.empty}>No rogue sessions established yet.</Text>
      ) : (
        sessionLogs.map((s, i) => (
          <View key={i} style={styles.sessionItem}>
            <Text style={styles.sessionText}>{s.msg}</Text>
          </View>
        ))
      )}

      <View style={styles.warning}>
        <Text style={styles.warningText}>
          ⚠️ Rogue gateway emulation hijacks end-device sessions. Only use on
          networks and devices you own or have explicit authorization to test.
          Requires a recovered JoinKey (use Key Search first).
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 8 },
  description: { color: '#888', fontSize: 12, marginBottom: 12, lineHeight: 18 },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 15, marginBottom: 6 },
  keyInput: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
              borderRadius: 4, padding: 10, fontSize: 13, fontFamily: 'monospace' },
  btnRow: { marginTop: 10 },
  startBtn: { backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88',
              borderRadius: 6, padding: 14, alignItems: 'center' },
  stopBtn: { backgroundColor: '#3a0a0a', borderWidth: 1, borderColor: '#ff6666',
             borderRadius: 6, padding: 14, alignItems: 'center' },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: 'bold' },
  activeBox: { backgroundColor: '#2a2a0a', borderWidth: 1, borderColor: '#ffaa00',
               borderRadius: 6, padding: 15, marginTop: 10 },
  activeText: { color: '#ffaa00', fontSize: 14, fontWeight: 'bold' },
  activeHint: { color: '#aaa', fontSize: 11, marginTop: 5 },
  empty: { color: '#888', fontSize: 13, fontStyle: 'italic' },
  sessionItem: { backgroundColor: '#1a1a2a', borderWidth: 1, borderColor: '#8866ff',
                 borderRadius: 4, padding: 10, marginBottom: 5 },
  sessionText: { color: '#8866ff', fontSize: 12, fontFamily: 'monospace' },
  warning: { backgroundColor: '#2a1a0a', borderWidth: 1, borderColor: '#ff6600',
             borderRadius: 6, padding: 10, marginTop: 15 },
  warningText: { color: '#ffaa44', fontSize: 11 },
});