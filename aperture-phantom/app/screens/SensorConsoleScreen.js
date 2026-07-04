/**
 * AperturePhantom — SensorConsoleScreen
 *
 * I²CCCI register read/write console for the attached camera sensor. Offers
 * an auto-detect button for common sensors (IMX219, OV5647, OV5640, etc.)
 * and a manual register read/write panel.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, TextInput, Button, FlatList, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function SensorConsoleScreen() {
  const { sensorScan, sensorRead, sensorWrite, sensorAutoconfig, log } = useDevice();
  const [addr, setAddr] = useState('0x36');
  const [reg, setReg] = useState('0x300A');
  const [val, setVal] = useState('0x0000');
  const [result, setResult] = useState('');

  const parse = (s) => parseInt(s.replace(/^0x/i, ''), 16);

  const onRead = async () => {
    try {
      await sensorRead(parse(addr), parse(reg), 2);
      setResult('read issued; see log');
    } catch (e) { Alert.alert('read failed', e.message); }
  };

  const onWrite = async () => {
    try {
      await sensorWrite(parse(addr), parse(reg), parse(val));
      setResult('write issued');
    } catch (e) { Alert.alert('write failed', e.message); }
  };

  const onScan = async () => { await sensorScan(); };
  const onAuto = async () => { await sensorAutoconfig(); };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Sensor I²CCCI Console</Text>

      <View style={styles.row}>
        <Button title="Auto-detect" onPress={onAuto} />
        <Button title="Bus scan"     onPress={onScan} />
      </View>

      <View style={styles.grid}>
        <Field label="Addr (7-bit hex)" value={addr} set={setAddr} />
        <Field label="Register (hex)"   value={reg}  set={setReg} />
        <Field label="Value (hex)"      value={val}  set={setVal} />
      </View>

      <View style={styles.row}>
        <Button title="Read"  onPress={onRead} />
        <Button title="Write" color="#a04040" onPress={onWrite} />
      </View>

      <Text style={styles.result}>{result}</Text>

      <Text style={styles.section}>Activity</Text>
      <FlatList
        data={log.slice(-12)}
        keyExtractor={(l, i) => String(i)}
        renderItem={({ item }) => <Text style={styles.log}>{item}</Text>}
      />

      <Text style={styles.note}>
        Common sensor addresses: IMX219=0x10, OV5647=0x36, OV5640=0x3C,
        OV5645=0x6C, IMX477=0x10, IMX296=0x10.
      </Text>
      <Text style={styles.author}>author: jayis1</Text>
    </View>
  );
}

function Field({ label, value, set }) {
  return (
    <View style={styles.field}>
      <Text style={styles.fieldLabel}>{label}</Text>
      <TextInput style={styles.in} value={value} onChangeText={set} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#101418' },
  h1: { color: '#d0e0f0', fontSize: 16, fontWeight: '600', marginBottom: 12 },
  row: { flexDirection: 'row', gap: 8, marginVertical: 8 },
  grid: { flexDirection: 'row', gap: 12, marginVertical: 8 },
  field: { flexDirection: 'column', flex: 1 },
  fieldLabel: { color: '#8aa0b8', fontSize: 10 },
  in: { borderWidth: 1, borderColor: '#304050', color: '#d0e0f0',
        paddingHorizontal: 8, paddingVertical: 6, borderRadius: 4, marginTop: 2,
        fontFamily: 'monospace' },
  result: { color: '#a0e0a0', fontSize: 12, marginTop: 4, fontFamily: 'monospace' },
  section: { color: '#a0b0c0', fontSize: 12, fontWeight: '600', marginTop: 12 },
  log: { color: '#7a9098', fontSize: 10, fontFamily: 'monospace' },
  note: { color: '#607080', fontSize: 10, marginTop: 16, fontStyle: 'italic' },
  author: { color: '#5a7088', fontSize: 10, marginTop: 8 },
});