// src/screens/GlitchScreen.tsx — VBUS voltage glitch sequencer.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { useState } from 'react';
import { View, Text, TextInput, Pressable, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../state/DeviceContext';
import { ConnectionBar } from '../components/ConnectionBar';

export default function GlitchScreen() {
  const { send } = useDevice();
  const [hi, setHi] = useState('20000');
  const [lo, setLo] = useState('5000');
  const [hiUs, setHiUs] = useState('1500');
  const [loUs, setLoUs] = useState('300');
  const [rep, setRep] = useState('3');

  const arm = () => {
    const H = parseInt(hi) || 0;
    const L = parseInt(lo) || 0;
    if (H > 48000 || L > 48000) {
      Alert.alert('Out of range', 'Voltage must be ≤ 48000 mV (PD 3.1 EPR max).');
      return;
    }
    send({ cmd: 'mode', mode: 'glitch' });
    send({ cmd: 'glitch', hi: H, lo: L, hi_us: parseInt(hiUs) || 1000, lo_us: parseInt(loUs) || 200, rep: parseInt(rep) || 1 });
    send({ cmd: 'arm' });
  };

  return (
    <ScrollView style={styles.container}>
      <ConnectionBar />
      <Text style={styles.h1}>VBUS Glitch Sequencer</Text>
      <Text style={styles.help}>
        Drive the victim to a high voltage, then abruptly drop VBUS to a low
        voltage for a short window, and repeat. The HRTIM on the STM32G474
        controls MOSFET switching with ~184 ps granularity.
      </Text>
      <Text style={styles.warn}>
        ⚠ Can cause over-voltage or brown-out faults. Use only on equipment you
        own or are authorized to test.
      </Text>

      <Text style={styles.label}>High voltage (mV)</Text>
      <TextInput style={styles.input} value={hi} onChangeText={setHi} keyboardType="numeric" />
      <Text style={styles.label}>Low voltage (mV)</Text>
      <TextInput style={styles.input} value={lo} onChangeText={setLo} keyboardType="numeric" />
      <Text style={styles.label}>High dwell (µs)</Text>
      <TextInput style={styles.input} value={hiUs} onChangeText={setHiUs} keyboardType="numeric" />
      <Text style={styles.label}>Low pulse (µs)</Text>
      <TextInput style={styles.input} value={loUs} onChangeText={setLoUs} keyboardType="numeric" />
      <Text style={styles.label}>Repeats</Text>
      <TextInput style={styles.input} value={rep} onChangeText={setRep} keyboardType="numeric" />

      <Pressable style={[styles.btn, { backgroundColor: '#d62828' }]} onPress={arm}>
        <Text style={styles.btnText}>⚠ Arm Glitch</Text>
      </Pressable>
      <Pressable style={[styles.btn, { backgroundColor: '#555' }]} onPress={() => send({ cmd: 'disarm' })}>
        <Text style={styles.btnText}>Disarm</Text>
      </Pressable>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d0d' },
  h1:        { color: '#eee', fontSize: 16, fontWeight: 'bold', padding: 12 },
  help:      { color: '#999', fontSize: 11, paddingHorizontal: 12, marginBottom: 8 },
  warn:      { color: '#f4a261', fontSize: 11, paddingHorizontal: 12, marginBottom: 12 },
  label:     { color: '#bbb', fontSize: 12, paddingHorizontal: 12, marginTop: 8 },
  input:     { backgroundColor: '#1a1a1a', color: '#eee', margin: 12, padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#333' },
  btn:       { margin: 12, marginTop: 16, padding: 14, borderRadius: 8, alignItems: 'center' },
  btnText:   { color: 'white', fontWeight: 'bold' },
});