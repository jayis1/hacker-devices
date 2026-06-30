// src/screens/SettingsScreen.js — Device configuration
// Author: jayis1
// License: MIT
import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, Picker } from 'react-native';
import { useDevice } from '../context/DeviceContext';

const MODES = [
  { label: 'Passive Sniff', value: 0 },
  { label: 'Key-Hunt', value: 1 },
  { label: 'Rogue Coordinator', value: 2 },
  { label: 'Selective Jam', value: 3 },
  { label: 'Cross-Channel Relay', value: 4 },
];

export default function SettingsScreen() {
  const { status, commands } = useDevice();
  const [mode, setMode] = useState(status.mode);
  const [channel, setChannel] = useState(String(status.channel));
  const [panFilter, setPanFilter] = useState('FFFF');
  const [euiFilter, setEuiFilter] = useState('0000000000000000');

  function applyMode(v) { setMode(v); commands.setMode(v); }
  function applyChannel(v) { setChannel(v); commands.setChannel(parseInt(v, 10)); }
  function applyPan() {
    const pan = parseInt(panFilter, 16);
    if (!isNaN(pan)) commands.setPanFilter(pan);
  }
  function applyEui() { commands.setEuiFilter(euiFilter); }

  return (
    <View style={styles.container}>
      <Text style={styles.header}>Settings</Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Operating Mode</Text>
        <Picker
          selectedValue={mode}
          onValueChange={applyMode}
          style={styles.picker}
          itemStyle={{ color: '#ddd', fontSize: 12 }}
        >
          {MODES.map((m) => <Picker.Item key={m.value} label={m.label} value={m.value} />)}
        </Picker>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Channel (11–26)</Text>
        <TextInput
          style={styles.input}
          value={channel}
          onChangeText={applyChannel}
          keyboardType="numeric"
        />
        <Text style={styles.hint}>{2405 + 5 * (parseInt(channel, 10) || 15)} MHz</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>PAN ID Filter</Text>
        <Text style={styles.hint}>FFFF = all PANs</Text>
        <View style={styles.row}>
          <TextInput style={styles.input} value={panFilter} onChangeText={setPanFilter} />
          <TouchableOpacity style={styles.btn} onPress={applyPan}>
            <Text style={styles.btnText}>Apply</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Source EUI-64 Filter</Text>
        <Text style={styles.hint}>0000000000000000 = any source</Text>
        <View style={styles.row}>
          <TextInput style={styles.input} value={euiFilter} onChangeText={setEuiFilter} />
          <TouchableOpacity style={styles.btn} onPress={applyEui}>
            <Text style={styles.btnText}>Apply</Text>
          </TouchableOpacity>
        </View>
      </View>

      <Text style={styles.footer}>ZIGBEE-PHANTOM · firmware by jayis1 · MIT</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 12 },
  header: { color: '#e85d2c', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 12 },
  cardTitle: { color: '#e85d2c', fontSize: 13, fontWeight: 'bold', marginBottom: 6 },
  hint: { color: '#888', fontSize: 10, marginTop: 4 },
  picker: { backgroundColor: '#0f0f23', color: '#ddd', borderRadius: 6 },
  input: { flex: 1, backgroundColor: '#0f0f23', color: '#ddd', borderRadius: 4, padding: 8, fontSize: 12 },
  row: { flexDirection: 'row', alignItems: 'center', marginTop: 6, gap: 8 },
  btn: { backgroundColor: '#e85d2c', padding: 10, borderRadius: 6 },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  footer: { color: '#666', fontSize: 10, textAlign: 'center', marginTop: 20 },
});