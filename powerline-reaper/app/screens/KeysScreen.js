// screens/KeysScreen.js — NMK dictionary attack control
// Author: jayis1
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Alert, FlatList } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import { MSG } from '../utils/protocol';

export default function KeysScreen() {
  const { nmkProgress, send } = useReaper();
  const [wordlistName, setWordlistName] = useState('rockyou.txt');

  const startOffline = () => {
    Alert.alert('NMK Crack (offline)', 'Start offline dictionary attack?', [
      { text: 'Cancel' },
      { text: 'Start', style: 'destructive', onPress: () => {
        // payload: 16-byte salt + 16-byte captured encrypted NEK
        const salt = new Uint8Array(16);
        salt.fill(0x48); // "HomePlugAV" placeholder
        const nekEnc = new Uint8Array(16);
        nekEnc.fill(0xAA);
        const p = new Uint8Array(32 + wordlistName.length);
        p.set(salt, 0);
        p.set(nekEnc, 16);
        for (let i = 0; i < wordlistName.length; i++) p[32 + i] = wordlistName.charCodeAt(i);
        send(MSG.NMK_CRACK_OFFLINE, p);
      }},
    ]);
  };

  const startOnline = () => {
    Alert.alert('NMK Crack (online)', 'Start online brute-force enrollment?', [
      { text: 'Cancel' },
      { text: 'Start', style: 'destructive', onPress: () => send(MSG.NMK_CRACK_ONLINE) },
    ]);
  };

  const abort = () => send(MSG.NMK_ABORT);

  const fmtNmk = (arr) => arr ? Array.from(arr).map((b) => b.toString(16).padStart(2,'0')).join(':') : '';

  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.header}>
        <Text style={styles.h1}>NMK Recovery</Text>
        <Text style={styles.sub}>HomePlug AV Network Membership Key</Text>
      </View>

      <View style={styles.statusBox}>
        <Text style={styles.h2}>Status</Text>
        <Row label="Running"    value={nmkProgress.running ? 'YES' : 'NO'} color={nmkProgress.running ? '#00ffaa' : '#888'} />
        <Row label="Tried"      value={String(nmkProgress.tried)} />
        <Row label="Rate"       value={`${nmkProgress.rate} H/s`} />
        <Row label="Recovered" value={nmkProgress.recovered ? 'YES' : 'NO'} color={nmkProgress.recovered ? '#00ffaa' : '#ff5555'} />
        {nmkProgress.recovered && <Text style={styles.nmk}>NMK: {fmtNmk(nmkProgress.nmk)}</Text>}
      </View>

      <View style={styles.inputBox}>
        <Text style={styles.label}>Wordlist (on SD)</Text>
        <TextInput style={styles.input} value={wordlistName} onChangeText={setWordlistName}
          placeholderTextColor="#444" />
      </View>

      <TouchableOpacity style={styles.btnGo} onPress={startOffline}>
        <Text style={styles.btnText}>Start Offline Dictionary</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.btnWarn} onPress={startOnline}>
        <Text style={styles.btnText}>Start Online Brute-Force</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.btnDanger} onPress={abort}>
        <Text style={styles.btnText}>Abort</Text>
      </TouchableOpacity>
    </View>
  );
}

const Row = ({ label, value, color = '#eee' }) => (
  <View style={styles.row}><Text style={styles.rowLabel}>{label}</Text><Text style={[styles.rowVal, { color }]}>{value}</Text></View>
);

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  header: { marginVertical: 12 },
  h1: { color: '#eee', fontSize: 20, fontWeight: 'bold' },
  h2: { color: '#00ffaa', fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  sub: { color: '#888', fontSize: 12 },
  statusBox: { backgroundColor: '#111', padding: 12, borderRadius: 6, marginBottom: 12 },
  inputBox: { backgroundColor: '#111', padding: 12, borderRadius: 6, marginBottom: 12 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  rowLabel: { color: '#888', fontSize: 12, fontFamily: 'monospace' },
  rowVal: { color: '#eee', fontSize: 12, fontFamily: 'monospace' },
  nmk: { color: '#00ffaa', fontSize: 12, fontFamily: 'monospace', marginTop: 8 },
  label: { color: '#888', fontSize: 11, marginBottom: 4 },
  input: { backgroundColor: '#0a0a0a', color: '#eee', borderWidth: 1, borderColor: '#222', borderRadius: 4, padding: 8, fontFamily: 'monospace' },
  btnGo: { backgroundColor: '#1a3a1a', padding: 14, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  btnWarn: { backgroundColor: '#3a2a1a', padding: 14, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  btnDanger: { backgroundColor: '#3a1a1a', padding: 14, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  btnText: { color: '#00ffaa', fontSize: 14, fontWeight: 'bold' },
});