/**
 * PulseScreen.js — Pulse configuration and firing for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Configure pulse width, current, polarity, count, and delay.
 * Fire single or multi-pulse sequences. Shows a timing diagram preview.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Switch, Alert, ScrollView } from 'react-native';
import { useBLE } from '../context/BLEContext';

export default function PulseScreen() {
  const { connectionState, sendCommand, telemetry } = useBLE();

  const [widthNs, setWidthNs] = useState('100000');     // 100 µs
  const [currentMa, setCurrentMa] = useState('5000');     // 5 A
  const [polarity, setPolarity] = useState(false);        // false=NORTH, true=SOUTH
  const [count, setCount] = useState('1');
  const [delayUs, setDelayUs] = useState('1000');        // 1 ms

  const handleFire = () => {
    if (connectionState !== 'connected') {
      Alert.alert('Not Connected', 'Please connect to MagLance first');
      return;
    }

    const w = parseInt(widthNs) || 0;
    const c = parseInt(currentMa) || 0;
    const n = parseInt(count) || 1;
    const d = parseInt(delayUs) || 0;
    const p = polarity ? 1 : 0;

    if (w < 100 || w > 100000000) {
      Alert.alert('Invalid Width', 'Pulse width must be 100 ns to 100 ms');
      return;
    }
    if (c < 1 || c > 80000) {
      Alert.alert('Invalid Current', 'Current must be 1 to 80000 mA');
      return;
    }

    Alert.alert(
      'Confirm Pulse',
      `Width: ${(w / 1000).toFixed(1)} µs\nCurrent: ${c} mA\nPolarity: ${p ? 'SOUTH' : 'NORTH'}\nCount: ${n}\nDelay: ${d} µs`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'FIRE',
          style: 'destructive',
          onPress: () => {
            const cmd = `PULSE ${w} ${c} ${p} ${n} ${d}\n`;
            sendCommand(cmd);
          }
        },
      ]
    );
  };

  const handleStop = () => {
    sendCommand('STOP\n');
  };

  const formatWidth = (ns) => {
    if (ns >= 1000000) return (ns / 1000000).toFixed(2) + ' ms';
    if (ns >= 1000) return (ns / 1000).toFixed(1) + ' µs';
    return ns + ' ns';
  };

  const estimateField = (currentMa) => {
    // Rough estimate: 50 mT at 5mm with 5A on standard tip
    return ((currentMa / 5000) * 50).toFixed(1) + ' mT';
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Pulse Mode</Text>
      <Text style={styles.subtitle}>Configure and fire magnetic field pulses</Text>

      {/* Pulse width */}
      <View style={styles.inputGroup}>
        <Text style={styles.label}>Pulse Width (ns)</Text>
        <TextInput
          style={styles.input}
          value={widthNs}
          onChangeText={setWidthNs}
          keyboardType="numeric"
          placeholder="100 - 100000000"
        />
        <Text style={styles.hint}>≈ {formatWidth(parseInt(widthNs) || 0)}</Text>
      </View>

      {/* Current */}
      <View style={styles.inputGroup}>
        <Text style={styles.label}>Coil Current (mA)</Text>
        <TextInput
          style={styles.input}
          value={currentMa}
          onChangeText={setCurrentMa}
          keyboardType="numeric"
          placeholder="1 - 80000"
        />
        <Text style={styles.hint}>≈ {estimateField(parseInt(currentMa) || 0)} field at 5mm</Text>
      </View>

      {/* Polarity */}
      <View style={styles.inputGroup}>
        <Text style={styles.label}>Polarity</Text>
        <View style={styles.polarityRow}>
          <Text style={styles.polarityLabel}>
            {polarity ? 'SOUTH (−)' : 'NORTH (+)'}
          </Text>
          <Switch
            value={polarity}
            onValueChange={setPolarity}
            trackColor={{ false: '#3498db', true: '#e74c3c' }}
            thumbColor="#ffffff"
          />
        </View>
      </View>

      {/* Count */}
      <View style={styles.inputGroup}>
        <Text style={styles.label}>Pulse Count</Text>
        <TextInput
          style={styles.input}
          value={count}
          onChangeText={setCount}
          keyboardType="numeric"
          placeholder="1 - 1000"
        />
      </View>

      {/* Delay */}
      <View style={styles.inputGroup}>
        <Text style={styles.label}>Inter-pulse Delay (µs)</Text>
        <TextInput
          style={styles.input}
          value={delayUs}
          onChangeText={setDelayUs}
          keyboardType="numeric"
          placeholder="0 - 1000000"
        />
      </View>

      {/* Timing diagram preview */}
      <View style={styles.diagramSection}>
        <Text style={styles.sectionTitle}>Timing Preview</Text>
        <View style={styles.diagram}>
          {Array.from({ length: Math.min(parseInt(count) || 1, 5) }).map((_, i) => (
            <View key={i} style={styles.pulseBar}>
              <View style={styles.pulseOn} />
              <View style={styles.pulseOff} />
            </View>
          ))}
          {parseInt(count) > 5 && (
            <Text style={styles.diagramMore}>... {parseInt(count) - 5} more pulses</Text>
          )}
        </View>
      </View>

      {/* Action buttons */}
      <TouchableOpacity style={styles.fireButton} onPress={handleFire}>
        <Text style={styles.fireButtonText}>⚡ FIRE PULSE</Text>
      </TouchableOpacity>

      <TouchableOpacity style={styles.stopButton} onPress={handleStop}>
        <Text style={styles.stopButtonText}>STOP</Text>
      </TouchableOpacity>

      {/* Safety warning */}
      <View style={styles.warningBox}>
        <Text style={styles.warningText}>
          ⚠ Ensure coil tip is positioned correctly.{'\n'}
          ⚠ Hold the dead-man's switch on the device.{'\n'}
          ⚠ High magnetic fields can damage electronics.
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
  label: { color: '#ecf0f1', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  input: { backgroundColor: '#16213e', color: '#ffffff', borderWidth: 1,
           borderColor: '#34495e', borderRadius: 8, padding: 12, fontSize: 16 },
  hint: { color: '#7f8c8d', fontSize: 11, marginTop: 4 },
  polarityRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  polarityLabel: { color: '#ecf0f1', fontSize: 16, fontWeight: 'bold' },
  diagramSection: { backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 16 },
  sectionTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  diagram: { flexDirection: 'column' },
  pulseBar: { flexDirection: 'row', height: 20, marginBottom: 4 },
  pulseOn: { width: 60, height: 20, backgroundColor: '#e74c3c', borderRadius: 2 },
  pulseOff: { flex: 1, height: 20, backgroundColor: '#1a1a2e', borderRadius: 2 },
  diagramMore: { color: '#95a5a6', fontSize: 10, marginTop: 4 },
  fireButton: { backgroundColor: '#e74c3c', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  fireButtonText: { color: '#ffffff', fontSize: 18, fontWeight: 'bold' },
  stopButton: { backgroundColor: '#34495e', padding: 12, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  stopButtonText: { color: '#ffffff', fontSize: 14, fontWeight: 'bold' },
  warningBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 16, borderWidth: 1, borderColor: '#e67e22' },
  warningText: { color: '#e67e22', fontSize: 11, lineHeight: 18 },
});