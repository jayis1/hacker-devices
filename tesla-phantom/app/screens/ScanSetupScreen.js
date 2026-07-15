/**
 * TeslaPhantom — Scan Setup Screen
 * Configure automated EMFI cartography scan parameters.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ScanSetupScreen() {
  const { connected, status, sendCommand } = useDevice();
  const [x0, setX0] = useState(0);
  const [y0, setY0] = useState(0);
  const [x1, setX1] = useState(10);
  const [y1, setY1] = useState(10);
  const [stepMm, setStepMm] = useState(0.5);
  const [zHeight, setZHeight] = useState(0.5);
  const [voltage, setVoltage] = useState(300);
  const [widthNs, setWidthNs] = useState(50);
  const [pulsesPerPoint, setPulsesPerPoint] = useState(3);
  const [scanning, setScanning] = useState(false);

  // Calculate total points
  const cols = Math.floor((x1 - x0) / stepMm) + 1;
  const rows = Math.floor((y1 - y0) / stepMm) + 1;
  const totalPoints = cols * rows;

  const handleStartScan = () => {
    if (totalPoints > 4000) {
      Alert.alert('Error', `Too many points (${totalPoints}). Max 4000.`);
      return;
    }
    Alert.alert(
      'Start Scan',
      `Scan ${totalPoints} points (${cols}×${rows}) at ${voltage}V?\n` +
      `Estimated time: ~${Math.ceil(totalPoints * pulsesPerPoint * 0.3 / 60)} min`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start',
          onPress: () => {
            setScanning(true);
            sendCommand({
              cmd: 'scan_start',
              x0: Math.round(x0 * 1000),
              y0: Math.round(y0 * 1000),
              x1: Math.round(x1 * 1000),
              y1: Math.round(y1 * 1000),
              step_mm: Math.round(stepMm * 1000)
            });
          }
        }
      ]
    );
  };

  const handleStopScan = () => {
    setScanning(false);
    sendCommand({ cmd: 'scan_stop' });
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Scan Area (mm)</Text>

      <View style={styles.gridRow}>
        <View style={styles.gridItem}>
          <Text style={styles.label}>X Start</Text>
          <TextInput style={styles.input} value={String(x0)}
            onChangeText={(v) => setX0(parseFloat(v) || 0)} keyboardType="numeric" />
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.label}>X End</Text>
          <TextInput style={styles.input} value={String(x1)}
            onChangeText={(v) => setX1(parseFloat(v) || 0)} keyboardType="numeric" />
        </View>
      </View>

      <View style={styles.gridRow}>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Y Start</Text>
          <TextInput style={styles.input} value={String(y0)}
            onChangeText={(v) => setY0(parseFloat(v) || 0)} keyboardType="numeric" />
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Y End</Text>
          <TextInput style={styles.input} value={String(y1)}
            onChangeText={(v) => setY1(parseFloat(v) || 0)} keyboardType="numeric" />
        </View>
      </View>

      <View style={styles.gridRow}>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Step (mm)</Text>
          <TextInput style={styles.input} value={String(stepMm)}
            onChangeText={(v) => setStepMm(parseFloat(v) || 0.1)} keyboardType="numeric" />
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Z Height</Text>
          <TextInput style={styles.input} value={String(zHeight)}
            onChangeText={(v) => setZHeight(parseFloat(v) || 0.5)} keyboardType="numeric" />
        </View>
      </View>

      <View style={styles.summaryBox}>
        <Text style={styles.summaryText}>Grid: {cols} × {rows}</Text>
        <Text style={styles.summaryText}>Total points: {totalPoints}</Text>
        <Text style={styles.summaryText}>Est. time: ~{Math.ceil(totalPoints * pulsesPerPoint * 0.3 / 60)} min</Text>
      </View>

      <Text style={styles.sectionTitle}>Pulse Parameters</Text>

      <View style={styles.gridRow}>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Voltage (V)</Text>
          <TextInput style={styles.input} value={String(voltage)}
            onChangeText={(v) => setVoltage(parseInt(v) || 50)} keyboardType="numeric" />
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Width (ns)</Text>
          <TextInput style={styles.input} value={String(widthNs)}
            onChangeText={(v) => setWidthNs(parseInt(v) || 5)} keyboardType="numeric" />
        </View>
      </View>

      <View style={styles.gridRow}>
        <View style={styles.gridItem}>
          <Text style={styles.label}>Pulses/Point</Text>
          <TextInput style={styles.input} value={String(pulsesPerPoint)}
            onChangeText={(v) => setPulsesPerPoint(parseInt(v) || 1)} keyboardType="numeric" />
        </View>
      </View>

      <TouchableOpacity
        style={[styles.button, scanning ? styles.stopBtn : styles.startBtn, !connected && styles.disabled]}
        onPress={scanning ? handleStopScan : handleStartScan}
        disabled={!connected}
      >
        <Text style={styles.buttonText}>
          {scanning ? '⏹ STOP SCAN' : '▶ START SCAN'}
        </Text>
      </TouchableOpacity>

      {scanning && (
        <Text style={styles.scanningText}>Scanning... Current: X:{status.position.x.toFixed(2)} Y:{status.position.y.toFixed(2)}</Text>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#e74c3c', marginTop: 16, marginBottom: 12 },
  gridRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  gridItem: { flex: 1, marginHorizontal: 4 },
  label: { fontSize: 12, color: '#95a5a6', marginBottom: 4 },
  input: { backgroundColor: '#1a1a2e', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, paddingHorizontal: 10, paddingVertical: 8, textAlign: 'center' },
  summaryBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginVertical: 12 },
  summaryText: { color: '#3498db', fontSize: 14, paddingVertical: 2 },
  button: { paddingVertical: 18, borderRadius: 8, marginTop: 16, marginBottom: 30, alignItems: 'center' },
  startBtn: { backgroundColor: '#2980b9' },
  stopBtn: { backgroundColor: '#c0392b' },
  disabled: { opacity: 0.4 },
  buttonText: { color: '#fff', fontSize: 18, fontWeight: 'bold' },
  scanningText: { color: '#f39c12', fontSize: 13, textAlign: 'center', marginBottom: 20 },
});