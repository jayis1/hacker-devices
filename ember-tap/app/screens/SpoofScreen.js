/**
 * SpoofScreen.js — Source Capabilities advertisement editor
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity, Alert, ScrollView,
} from 'react-native';
import { CMD, EmberTapConnection } from '../utils/protocol';

export default function SpoofScreen() {
  const [voltage, setVoltage] = useState('5000');
  const [current, setCurrent] = useState('3000');
  const [conn, setConn] = useState(null);
  const [lastAdvertised, setLastAdvertised] = useState(null);

  const advertise = async () => {
    if (!conn) {
      Alert.alert('Not connected', 'Connect from Dashboard first.');
      return;
    }
    const mv = parseInt(voltage, 10);
    const ma = parseInt(current, 10);
    if (mv < 5000 || mv > 20000) {
      Alert.alert('Invalid voltage', 'Voltage must be 5000–20000 mV');
      return;
    }
    if (ma < 500 || ma > 5000) {
      Alert.alert('Invalid current', 'Current must be 500–5000 mA');
      return;
    }
    const { encodeSpoofSrc } = require('../utils/protocol');
    await conn.send(CMD.SPOOF_SRC, encodeSpoofSrc(mv, ma));
    setLastAdvertised({ mv, ma });
  };

  const presets = [
    { label: '5V / 3A (safe)', mv: 5000, ma: 3000 },
    { label: '9V / 3A', mv: 9000, ma: 3000 },
    { label: '15V / 3A', mv: 15000, ma: 3000 },
    { label: '20V / 5A (dangerous)', mv: 20000, ma: 5000 },
    { label: '20V / 100mA (undervolt)', mv: 20000, ma: 100 },
  ];

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Source Spoof</Text>
      <Text style={styles.desc}>
        Advertise a custom Fixed PDO to the DUT. The DUT will accept or reject
        based on its own sink capabilities. Use with caution — an unprotected
        sink may be overvolted.
      </Text>

      <View style={styles.field}>
        <Text style={styles.label}>Voltage (mV)</Text>
        <TextInput
          style={styles.input}
          value={voltage}
          onChangeText={setVoltage}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.field}>
        <Text style={styles.label}>Current (mA)</Text>
        <TextInput
          style={styles.input}
          value={current}
          onChangeText={setCurrent}
          keyboardType="numeric"
        />
      </View>

      <Text style={styles.presetTitle}>Presets:</Text>
      {presets.map((p) => (
        <TouchableOpacity
          key={p.label}
          style={styles.presetBtn}
          onPress={() => { setVoltage(String(p.mv)); setCurrent(String(p.ma)); }}
        >
          <Text style={styles.presetText}>{p.label}</Text>
        </TouchableOpacity>
      ))}

      <TouchableOpacity style={styles.advertiseBtn} onPress={advertise}>
        <Text style={styles.btnText}>Advertise</Text>
      </TouchableOpacity>

      {lastAdvertised && (
        <Text style={styles.confirmText}>
          ✓ Advertised {lastAdvertised.mv}mV / {lastAdvertised.ma}mA
        </Text>
      )}

      <Text style={styles.disclaimer}>
        ⚠ Overvoltage can destroy the DUT. Authorized testing only.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { color: '#FF6600', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  desc: { color: '#aaa', fontSize: 12, marginBottom: 20 },
  field: { marginBottom: 16 },
  label: { color: '#888', fontSize: 14, marginBottom: 4 },
  input: {
    backgroundColor: '#16213e', color: '#fff', borderRadius: 6,
    padding: 12, fontSize: 16, borderWidth: 1, borderColor: '#333',
  },
  presetTitle: { color: '#888', fontSize: 14, marginBottom: 8 },
  presetBtn: {
    backgroundColor: '#16213e', padding: 12, borderRadius: 6,
    marginBottom: 6, borderWidth: 1, borderColor: '#333',
  },
  presetText: { color: '#fff', fontSize: 14 },
  advertiseBtn: {
    backgroundColor: '#FF6600', padding: 16, borderRadius: 8,
    alignItems: 'center', marginTop: 12,
  },
  btnText: { color: '#fff', fontSize: 18, fontWeight: 'bold' },
  confirmText: { color: '#4CAF50', fontSize: 14, marginTop: 12, textAlign: 'center' },
  disclaimer: { color: '#FFAA00', fontSize: 11, marginTop: 20, fontStyle: 'italic' },
});

/* end of file — author: jayis1 */