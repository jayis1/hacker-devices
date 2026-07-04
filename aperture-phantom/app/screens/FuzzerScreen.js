/**
 * AperturePhantom — FuzzerScreen
 *
 * CSI-2 receiver/ISP-driver fuzzer. Pick corruption strategies (bitmap),
 * iteration count, and delay; the device will emit malformed CSI-2 packets
 * toward the host receiver. Use only against systems you own/are
 * authorized to test.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, TextInput, Button, StyleSheet, Alert, Switch } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const STRATEGIES = [
  { id: 0x01, label: 'Short line' },
  { id: 0x02, label: 'Bad data type' },
  { id: 0x04, label: 'Bad virtual channel' },
  { id: 0x08, label: 'Oversize payload' },
  { id: 0x10, label: 'Bad header CRC' },
  { id: 0x20, label: 'Missing FS' },
  { id: 0x40, label: 'Missing FE' },
  { id: 0x80, label: 'Stray short packet' },
];

export default function FuzzerScreen() {
  const { fuzzStart, fuzzStop, log } = useDevice();
  const [mask, setMask] = useState(0x01);
  const [count, setCount] = useState('50');
  const [delay, setDelay] = useState('100');

  const toggle = (bit, on) => {
    setMask((m) => on ? (m | bit) : (m & ~bit));
  };

  const onStart = async () => {
    const n = parseInt(count, 10) || 1;
    const d = parseInt(delay, 10) || 100;
    await fuzzStart(mask, n, d);
    Alert.alert('Fuzz running', `strat=0x${mask.toString(16)} count=${n} delay=${d}ms`);
  };

  const onStop = async () => { await fuzzStop(); };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>CSI-2 Receiver Fuzzer</Text>
      <Text style={styles.warn}>
        ⚠ Emits malformed CSI-2 toward the host receiver/ISP driver to find
        parsing bugs. May cause kernel panics. Authorized test benches only.
      </Text>

      <Text style={styles.section}>Strategies</Text>
      {STRATEGIES.map((s) => (
        <View key={s.id} style={styles.stratRow}>
          <Text style={styles.stratLabel}>{s.label}</Text>
          <Switch
            value={!!(mask & s.id)}
            onValueChange={(on) => toggle(s.id, on)}
            trackColor={{ true: '#4080d0', false: '#303030' }}
          />
        </View>
      ))}

      <View style={styles.row}>
        <Field label="Count"     value={count} set={setCount} />
        <Field label="Delay ms"  value={delay} set={setDelay} />
      </View>

      <View style={styles.row}>
        <Button title="Start" color="#a04040" onPress={onStart} />
        <Button title="Stop"  onPress={onStop} />
      </View>

      <Text style={styles.section}>Activity</Text>
      {log.slice(-6).map((l, i) => (
        <Text key={i} style={styles.log}>{l}</Text>
      ))}

      <Text style={styles.author}>author: jayis1</Text>
    </View>
  );
}

function Field({ label, value, set }) {
  return (
    <View style={styles.field}>
      <Text style={styles.fieldLabel}>{label}</Text>
      <TextInput style={styles.in} value={value} keyboardType="numeric"
                 onChangeText={set} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418' },
  h1: { color: '#d0e0f0', fontSize: 16, fontWeight: '600', marginBottom: 8 },
  warn: { color: '#e0a040', fontSize: 11, fontStyle: 'italic', marginBottom: 12 },
  section: { color: '#a0b0c0', fontSize: 12, fontWeight: '600', marginTop: 12 },
  stratRow: { flexDirection: 'row', justifyContent: 'space-between',
              alignItems: 'center', paddingVertical: 6 },
  stratLabel: { color: '#d0e0f0', fontSize: 13 },
  row: { flexDirection: 'row', gap: 12, marginVertical: 8 },
  field: { flexDirection: 'column', flex: 1 },
  fieldLabel: { color: '#8aa0b8', fontSize: 10 },
  in: { borderWidth: 1, borderColor: '#304050', color: '#d0e0f0',
        paddingHorizontal: 8, paddingVertical: 6, borderRadius: 4, marginTop: 2 },
  log: { color: '#7a9098', fontSize: 10, fontFamily: 'monospace' },
  author: { color: '#5a7088', fontSize: 10, marginTop: 16 },
});