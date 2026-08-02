/**
 * SettingsScreen.js — Phantom runtime configuration & safety controls.
 *
 * Author: jayis1
 * License: GPLv3
 */
import React, { useState } from 'react';
import { View, Text, Button, TextInput, StyleSheet, Alert, Switch, ScrollView } from 'react-native';
import { usePhantom } from '../src/PhantomContext';

export default function SettingsScreen() {
  const { sendCommand, connected, events } = usePhantom();
  const [ackTimeout, setAckTimeout] = useState('1500');
  const [mstpAddr, setMstpAddr] = useState('5');
  const [bridge, setBridge] = useState(false);

  const cmd = (obj) => sendCommand(obj).then((ok) =>
    Alert.alert(ok ? 'ok' : 'failed', ok ? JSON.stringify(obj) : 'BLE write failed'));

  return (
    <ScrollView style={s.wrap}>
      <Text style={s.h2}>Settings</Text>

      <Text style={s.section}>Mode</Text>
      <View style={s.row}>
        <Button title="Passive" onPress={() => cmd({ op: 'passive' })} />
        <Button title="Discover" onPress={() => cmd({ op: 'whois', low: 0, high: 4194303 })} />
      </View>
      <View style={s.row}>
        <Button title="Bridge" color="#a663cc" onPress={() => cmd({ op: 'bridge', a1: bridge ? 0 : 1 })} />
        <Switch value={bridge} onValueChange={setBridge} />
      </View>

      <Text style={s.section}>MS/TP</Text>
      <Text style={s.label}>Master address (0..127)</Text>
      <TextInput style={s.in} value={mstpAddr} onChangeText={setMstpAddr} keyboardType="numeric" />
      <Button title="Join MS/TP ring" onPress={() => cmd({ op: 'join_mstp', a1: parseInt(mstpAddr, 10) })} />

      <Text style={s.section}>Watchdog</Text>
      <Text style={s.label}>ACK timeout (ms)</Text>
      <TextInput style={s.in} value={ackTimeout} onChangeText={setAckTimeout} keyboardType="numeric" />
      <Button title="Update timeout" onPress={() => cmd({ op: 'ack_timeout', a1: parseInt(ackTimeout, 10) })} />

      <Text style={s.section}>Event log</Text>
      {events.slice(0, 30).map((e, i) => (
        <Text key={i} style={s.log}>{new Date(e.t).toLocaleTimeString()} {JSON.stringify(e.msg)}</Text>
      ))}

      <Text style={s.footer}>BACnet Phantom — author: jayis1 — GPLv3 / CERN-OHL-W</Text>
    </ScrollView>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, padding: 20, backgroundColor: '#0f1115' },
  h2: { fontSize: 20, fontWeight: 'bold', color: '#118ab2', marginTop: 8, marginBottom: 8 },
  section: { color: '#ccc', marginTop: 16, marginBottom: 4, fontWeight: 'bold' },
  row: { flexDirection: 'row', alignItems: 'center', gap: 12, marginVertical: 6 },
  label: { color: '#888', fontSize: 12 },
  in: { color: '#fff', borderBottomWidth: 1, borderColor: '#333', padding: 8, marginTop: 4 },
  log: { color: '#06d6a0', fontSize: 10, fontFamily: 'monospace' },
  footer: { color: '#444', fontSize: 10, marginTop: 24, textAlign: 'center' },
});