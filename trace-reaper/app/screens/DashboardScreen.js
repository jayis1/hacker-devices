/**
 * TRACE-REAPER — Dashboard screen
 *
 * Shows device connection state, current mode, uptime, battery, and the
 * live trace count. Buttons to connect, authenticate, and arm/stop.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useContext, useEffect, useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import { ConnectionContext, modeName, CMD } from '../utils/protocol';

export default function DashboardScreen() {
  const conn = useContext(ConnectionContext);
  const [snap, setSnap] = useState(conn ? conn._snapshot() : null);

  useEffect(() => {
    if (!conn) return;
    return conn.subscribe(() => setSnap(conn._snapshot()));
  }, [conn]);

  const connect = async () => {
    try { await conn.startScan(); }
    catch (e) { Alert.alert('Connect failed', String(e)); }
  };

  const auth = async () => {
    Alert.prompt(
      'Authenticate',
      'Enter the 6-digit passkey shown on the device OLED',
      async (pk) => { try { await conn.authenticate(pk || '000000'); } catch (e) { Alert.alert('Auth failed', String(e)); } },
      'plain-text',
      '000000',
      'numeric',
    );
  };

  const arm = async () => { try { await conn.arm(); } catch (e) { Alert.alert('Arm failed', String(e)); } };
  const stop = async () => { try { await conn.stop(); } catch (e) { Alert.alert('Stop failed', String(e)); } };

  const s = snap || {};
  const st = s.status || {};

  return (
    <View style={styles.container}>
      <Text style={styles.title}>TRACE-REAPER</Text>
      <Text style={styles.subtitle}>Portable Power-Side-Channel Analyzer</Text>
      <Text style={styles.author}>by jayis1 · GPL-2.0</Text>

      <View style={styles.card}>
        <Row label="Connected"      value={s.connected ? 'yes' : 'no'} />
        <Row label="Authenticated"  value={s.authenticated ? 'yes' : 'no'} />
        <Row label="Mode"           value={modeName(st.mode || 0)} />
        <Row label="Uptime"         value={`${st.uptime_s || 0} s`} />
        <Row label="Battery"        value={`${st.battery_pct || 0}%`} />
        <Row label="Traces captured" value={`${st.traces || 0}`} />
        {s.tampered && <Text style={styles.tamper}>⚠ TAMPER — device zeroized</Text>}
        {s.fault ? <Text style={styles.fault}>⚠ {s.fault}</Text> : null}
      </View>

      <View style={styles.buttons}>
        <Btn label="Connect"      onPress={connect} disabled={s.connected} />
        <Btn label="Authenticate" onPress={auth}     disabled={!s.connected || s.authenticated} />
        <Btn label="Arm"          onPress={arm}      disabled={!s.authenticated || st.mode !== 1} />
        <Btn label="Stop"         onPress={stop}     disabled={st.mode !== 3} />
      </View>

      <Text style={styles.disclaimer}>
        For authorized security research only. Use solely on hardware you own
        or are explicitly authorized to test.
      </Text>
    </View>
  );
}

function Row({ label, value }) {
  return (
    <View style={styles.row}>
      <Text style={styles.rowLabel}>{label}</Text>
      <Text style={styles.rowValue}>{value}</Text>
    </View>
  );
}

function Btn({ label, onPress, disabled }) {
  return (
    <TouchableOpacity onPress={onPress} disabled={disabled}
      style={[styles.btn, disabled && styles.btnDisabled]}>
      <Text style={styles.btnText}>{label}</Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1622', padding: 16 },
  title: { color: '#00e5ff', fontSize: 24, fontWeight: '700', marginTop: 16 },
  subtitle: { color: '#9fb1c7', fontSize: 13, marginTop: 2 },
  author: { color: '#5d7088', fontSize: 11, marginTop: 2 },
  card: { backgroundColor: '#13243a', borderRadius: 8, padding: 14, marginTop: 18 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  rowLabel: { color: '#9fb1c7', fontSize: 13 },
  rowValue: { color: '#e6edf3', fontSize: 13, fontWeight: '600' },
  buttons: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 18, gap: 8 },
  btn: { backgroundColor: '#1f6feb', padding: 10, borderRadius: 6, marginRight: 8, marginBottom: 8 },
  btnDisabled: { backgroundColor: '#2a3a52' },
  btnText: { color: '#fff', fontSize: 13, fontWeight: '600' },
  tamper: { color: '#e74c3c', marginTop: 10, fontWeight: '700' },
  fault: { color: '#f1c40f', marginTop: 6 },
  disclaimer: { color: '#5d7088', fontSize: 10, marginTop: 24, lineHeight: 14 },
});