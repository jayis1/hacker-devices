// screens/CaptureScreen.js — promiscuous capture control + live frame feed
// Author: jayis1
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Switch } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import FrameLog from '../components/FrameLog';
import { MSG } from '../utils/protocol';

export default function CaptureScreen() {
  const { send, frames } = useReaper();
  const [running, setRunning] = useState(false);
  const [filterMac, setFilterMac] = useState('');
  const [filterEt, setFilterEt] = useState('');

  const toggle = () => {
    if (running) {
      send(MSG.STOP_SNIFF);
      setRunning(false);
    } else {
      send(MSG.START_SNIFF);
      setRunning(true);
    }
  };

  const applyFilter = () => {
    const macBytes = filterMac ? filterMac.split(':').map((h) => parseInt(h, 16)) : null;
    const et = filterEt ? parseInt(filterEt, 16) : 0;
    const payload = new Uint8Array(8);
    if (macBytes) payload.set(macBytes.slice(0, 6), 0);
    payload[6] = et & 0xFF;
    payload[7] = (et >> 8) & 0xFF;
    send(MSG.SET_FILTER, payload);
  };

  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.header}>
        <Text style={styles.h1}>Capture</Text>
        <Text style={styles.sub}>{frames.length} frames in buffer</Text>
      </View>

      <TouchableOpacity style={[styles.btn, running ? styles.btnStop : styles.btnGo]} onPress={toggle}>
        <Text style={styles.btnText}>{running ? '■ STOP' : '▶ START SNIFF'}</Text>
      </TouchableOpacity>

      <View style={styles.filterBox}>
        <Text style={styles.h2}>Filter</Text>
        <Text style={styles.label}>MAC (xx:xx:xx:xx:xx:xx)</Text>
        <TextInput style={styles.input} value={filterMac} onChangeText={setFilterMac}
          placeholder="any" placeholderTextColor="#444" autoCapitalize="none" />
        <Text style={styles.label}>EtherType (hex)</Text>
        <TextInput style={styles.input} value={filterEt} onChangeText={setFilterEt}
          placeholder="0x0800" placeholderTextColor="#444" />
        <TouchableOpacity style={styles.btnSm} onPress={applyFilter}>
          <Text style={styles.btnTextSm}>Apply</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.logBox}>
        <Text style={styles.h2}>Live Frames</Text>
        <FrameLog />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  header: { marginVertical: 12 },
  h1: { color: '#eee', fontSize: 20, fontWeight: 'bold' },
  h2: { color: '#00ffaa', fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  sub: { color: '#888', fontSize: 12 },
  btn: { padding: 16, borderRadius: 6, alignItems: 'center', marginTop: 8 },
  btnGo: { backgroundColor: '#1a3a1a' },
  btnStop: { backgroundColor: '#3a1a1a' },
  btnText: { color: '#00ffaa', fontSize: 16, fontWeight: 'bold' },
  filterBox: { backgroundColor: '#111', padding: 10, borderRadius: 6, marginTop: 12 },
  label: { color: '#888', fontSize: 11, marginTop: 6 },
  input: { backgroundColor: '#0a0a0a', color: '#eee', borderWidth: 1, borderColor: '#222', borderRadius: 4, padding: 8, marginTop: 4, fontFamily: 'monospace' },
  btnSm: { backgroundColor: '#1a2a1a', padding: 8, borderRadius: 4, marginTop: 8, alignItems: 'center' },
  btnTextSm: { color: '#00ffaa', fontSize: 12 },
  logBox: { flex: 1, backgroundColor: '#111', padding: 10, borderRadius: 6, marginTop: 12 },
});