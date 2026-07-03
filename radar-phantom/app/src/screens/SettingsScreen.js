/**
 * SettingsScreen.js — calibration, safety interlocks, firmware update
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, Switch, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function SettingsScreen() {
  const { commands, battery, firmware, disconnect } = useDevice();
  const [power, setPower] = useState('180');
  const [safetyAck, setSafetyAck] = useState(false);
  const [loFreq, setLoFreq] = useState('38500000000');

  const applyPower = async () => {
    const code = parseInt(power, 10);
    if (code > 200) {
      Alert.alert('Power ceiling', 'Max RF power code is 200 (safety).');
      return;
    }
    await commands.setPower(code);
  };

  const applyLo = async () => {
    // In a full build this sends a dedicated LO_TUNE opcode (0x0C).
    Alert.alert('LO tune', `Would set LO to ${parseInt(loFreq,10)/1e9} GHz (firmware opcode 0x0C)`);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <Text style={styles.subtitle}>Calibration & safety interlocks</Text>

      <View style={styles.section}>
        <Text style={styles.sectionLabel}>DEVICE</Text>
        <View style={styles.row}><Text style={styles.key}>Firmware</Text><Text style={styles.val}>{(firmware>>8)}.{firmware&0xFF}</Text></View>
        <View style={styles.row}><Text style={styles.key}>Battery</Text><Text style={styles.val}>{battery}%</Text></View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionLabel}>RF POWER (gain code)</Text>
        <View style={styles.row}>
          <TextInput
            style={styles.input}
            value={power}
            onChangeText={setPower}
            keyboardType="numeric"
          />
          <TouchableOpacity style={styles.btn} onPress={applyPower}>
            <Text style={styles.btnText}>Apply</Text>
          </TouchableOpacity>
        </View>
        <Text style={styles.hint}>Safety ceiling: 200 (programmable attenuator code)</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionLabel}>LO FREQUENCY (Hz)</Text>
        <View style={styles.row}>
          <TextInput
            style={styles.input}
            value={loFreq}
            onChangeText={setLoFreq}
            keyboardType="numeric"
          />
          <TouchableOpacity style={styles.btn} onPress={applyLo}>
            <Text style={styles.btnText}>Tune</Text>
          </TouchableOpacity>
        </View>
        <Text style={styles.hint}>Sub-harmonic LO range: 38.0–40.5 GHz</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionLabel}>SAFETY INTERLOCK</Text>
        <View style={styles.row}>
          <Text style={styles.key}>I acknowledge authorized use only</Text>
          <Switch value={safetyAck} onValueChange={setSafetyAck} />
        </View>
        {!safetyAck && <Text style={styles.warn}>⚠ Arm is blocked until acknowledged</Text>}
      </View>

      <TouchableOpacity style={styles.disconnectBtn} onPress={disconnect}>
        <Text style={styles.disconnectText}>Disconnect</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>RadarPhantom v1.0 · jayis1 · authorized research only</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 16 },
  title: { color: '#00ff9f', fontSize: 22, fontWeight: 'bold', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 12, marginBottom: 20 },
  section: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginBottom: 16 },
  sectionLabel: { color: '#666', fontSize: 11, fontWeight: 'bold', marginBottom: 10 },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginBottom: 8 },
  key: { color: '#ccc', fontSize: 13, flex: 1 },
  val: { color: '#00ff9f', fontSize: 14, fontWeight: 'bold' },
  input: { backgroundColor: '#0f0f1a', color: '#fff', padding: 8, borderRadius: 6, flex: 1, marginRight: 8, fontSize: 13 },
  btn: { backgroundColor: '#2a4e2a', padding: 10, borderRadius: 6 },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  hint: { color: '#666', fontSize: 10, marginTop: 6, fontStyle: 'italic' },
  warn: { color: '#ffaa00', fontSize: 11, marginTop: 8 },
  disconnectBtn: { backgroundColor: '#4e2a2a', padding: 14, borderRadius: 8, alignItems: 'center', marginTop: 8 },
  disconnectText: { color: '#ff5555', fontWeight: 'bold' },
  footer: { color: '#444', fontSize: 10, textAlign: 'center', marginTop: 20 },
});