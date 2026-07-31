/**
 * SweepScreen.js — Frequency sweep configuration for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Configure frequency sweep parameters: start/stop frequency, number of
 * steps, dwell time per step, and coil current. Start/stop the sweep.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, Alert, ScrollView, Switch } from 'react-native';
import { useBLE } from '../context/BLEContext';

export default function SweepScreen() {
  const { connectionState, sendCommand } = useBLE();

  const [startHz, setStartHz] = useState('1000');     // 1 kHz
  const [stopHz, setStopHz] = useState('100000');      // 100 kHz
  const [steps, setSteps] = useState('50');
  const [dwellMs, setDwellMs] = useState('100');
  const [currentMa, setCurrentMa] = useState('3000');  // 3 A
  const [isLinear, setIsLinear] = useState(true);
  const [isRunning, setIsRunning] = useState(false);

  const handleStart = () => {
    if (connectionState !== 'connected') {
      Alert.alert('Not Connected', 'Please connect to MagLance first');
      return;
    }

    const start = parseInt(startHz) || 0;
    const stop = parseInt(stopHz) || 0;
    const s = parseInt(steps) || 0;
    const dwell = parseInt(dwellMs) || 0;
    const current = parseInt(currentMa) || 0;

    if (start < 1 || stop < start) {
      Alert.alert('Invalid Range', 'Start frequency must be < stop frequency');
      return;
    }
    if (s < 1 || s > 1000) {
      Alert.alert('Invalid Steps', 'Steps must be 1-1000');
      return;
    }
    if (current > 8000) {
      Alert.alert('Invalid Current', 'DC mode max is 8000 mA');
      return;
    }

    Alert.alert(
      'Confirm Sweep',
      `Range: ${start} Hz → ${stop} Hz\nSteps: ${s}\nDwell: ${dwell} ms/step\nCurrent: ${current} mA\nType: ${isLinear ? 'Linear' : 'Log'}`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start Sweep',
          style: 'destructive',
          onPress: () => {
            sendCommand(`SWEEP ${start} ${stop} ${s} ${dwell} ${current}\n`);
            setIsRunning(true);
          }
        },
      ]
    );
  };

  const handleStop = () => {
    sendCommand('STOP\n');
    setIsRunning(false);
  };

  // Calculate sweep duration
  const totalDuration = ((parseInt(steps) || 0) * (parseInt(dwellMs) || 0)) / 1000;
  const stepSize = isLinear
    ? ((parseInt(stopHz) || 0) - (parseInt(startHz) || 0)) / (parseInt(steps) || 1)
    : 0; // Logarithmic calculated differently

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Sweep Mode</Text>
      <Text style={styles.subtitle}>Frequency-swept AC magnetic field generation</Text>

      <View style={styles.inputGroup}>
        <Text style={styles.label}>Start Frequency (Hz)</Text>
        <TextInput style={styles.input} value={startHz} onChangeText={setStartHz}
          keyboardType="numeric" placeholder="1 - 100000" />
      </View>

      <View style={styles.inputGroup}>
        <Text style={styles.label}>Stop Frequency (Hz)</Text>
        <TextInput style={styles.input} value={stopHz} onChangeText={setStopHz}
          keyboardType="numeric" placeholder="1 - 100000" />
      </View>

      <View style={styles.inputRow}>
        <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
          <Text style={styles.label}>Steps</Text>
          <TextInput style={styles.input} value={steps} onChangeText={setSteps}
            keyboardType="numeric" placeholder="1-1000" />
        </View>
        <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
          <Text style={styles.label}>Dwell (ms)</Text>
          <TextInput style={styles.input} value={dwellMs} onChangeText={setDwellMs}
            keyboardType="numeric" placeholder="1-10000" />
        </View>
      </View>

      <View style={styles.inputGroup}>
        <Text style={styles.label}>Coil Current (mA, max 8000)</Text>
        <TextInput style={styles.input} value={currentMa} onChangeText={setCurrentMa}
          keyboardType="numeric" placeholder="1 - 8000" />
      </View>

      <View style={styles.inputGroup}>
        <Text style={styles.label}>Sweep Type</Text>
        <View style={styles.polarityRow}>
          <Text style={styles.polarityLabel}>{isLinear ? 'Linear' : 'Logarithmic'}</Text>
          <Switch value={isLinear} onValueChange={setIsLinear}
            trackColor={{ false: '#9b59b6', true: '#3498db' }} thumbColor="#ffffff" />
        </View>
      </View>

      {/* Summary */}
      <View style={styles.summaryBox}>
        <Text style={styles.summaryTitle}>Sweep Summary</Text>
        <Text style={styles.summaryText}>Duration: {totalDuration.toFixed(1)} s</Text>
        <Text style={styles.summaryText}>Step size: {isLinear ? stepSize.toFixed(0) + ' Hz' : 'Logarithmic'}</Text>
        <Text style={styles.summaryText}>Total steps: {steps}</Text>
      </View>

      {/* Action buttons */}
      {!isRunning ? (
        <TouchableOpacity style={styles.startButton} onPress={handleStart}>
          <Text style={styles.startButtonText}>▶ START SWEEP</Text>
        </TouchableOpacity>
      ) : (
        <TouchableOpacity style={styles.stopButton} onPress={handleStop}>
          <Text style={styles.stopButtonText}>■ STOP SWEEP</Text>
        </TouchableOpacity>
      )}

      <View style={styles.warningBox}>
        <Text style={styles.warningText}>
          ⚠ Sweep mode generates AC magnetic fields.{'\n'}
          ⚠ May interfere with nearby electronic devices.{'\n'}
          ⚠ Keep away from magnetic storage media.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#e74c3c', marginBottom: 4 },
  subtitle: { color: '#95a5a6', fontSize: 12, marginBottom: 16 },
  inputGroup: { marginBottom: 16 },
  inputRow: { flexDirection: 'row' },
  label: { color: '#ecf0f1', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  input: { backgroundColor: '#16213e', color: '#ffffff', borderWidth: 1,
           borderColor: '#34495e', borderRadius: 8, padding: 12, fontSize: 16 },
  polarityRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  polarityLabel: { color: '#ecf0f1', fontSize: 16, fontWeight: 'bold' },
  summaryBox: { backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 16 },
  summaryTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  summaryText: { color: '#ecf0f1', fontSize: 13, marginBottom: 4 },
  startButton: { backgroundColor: '#27ae60', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  startButtonText: { color: '#ffffff', fontSize: 18, fontWeight: 'bold' },
  stopButton: { backgroundColor: '#c0392b', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  stopButtonText: { color: '#ffffff', fontSize: 18, fontWeight: 'bold' },
  warningBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 16, borderWidth: 1, borderColor: '#e67e22' },
  warningText: { color: '#e67e22', fontSize: 11, lineHeight: 18 },
});