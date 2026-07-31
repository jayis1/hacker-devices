/**
 * SenseScreen.js — Passive magnetic field sensing for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Passive-only mode: streams magnetometer data from all three RM3100
 * sensors. Displays real-time field magnitude, gradient, and a simple
 * waveform plot. Useful for magnetic side-channel analysis and
 * current waveform reconstruction through enclosures.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, ScrollView, View } from 'react-native';
import { useBLE } from '../context/BLEContext';

export default function SenseScreen() {
  const { connectionState, sendCommand, telemetry } = useBLE();
  const [rateHz, setRateHz] = useState('150');
  const [durationS, setDurationS] = useState('10');
  const [isStreaming, setIsStreaming] = useState(false);
  const [fieldHistory, setFieldHistory] = useState([]);
  const streamTimer = useRef(null);

  // Record field readings for waveform display
  useEffect(() => {
    if (isStreaming) {
      const reading = {
        x0: telemetry.fieldX[0], y0: telemetry.fieldY[0], z0: telemetry.fieldZ[0],
        x1: telemetry.fieldX[1], y1: telemetry.fieldY[1], z1: telemetry.fieldZ[1],
        x2: telemetry.fieldX[2], y2: telemetry.fieldY[2], z2: telemetry.fieldZ[2],
        timestamp: Date.now(),
      };
      setFieldHistory(prev => [...prev.slice(-100), reading]); // Keep last 100
    }
  }, [telemetry, isStreaming]);

  const handleStartStream = () => {
    if (connectionState !== 'connected') {
      return;
    }
    const rate = parseInt(rateHz) || 150;
    const dur = parseInt(durationS) || 10;
    sendCommand(`SENSE ${rate} ${dur}\n`);
    setIsStreaming(true);

    // Stop after duration
    streamTimer.current = setTimeout(() => {
      setIsStreaming(false);
    }, dur * 1000);
  };

  const handleStopStream = () => {
    sendCommand('STOP\n');
    setIsStreaming(false);
    if (streamTimer.current) clearTimeout(streamTimer.current);
  };

  // Calculate vector magnitude for each sensor
  const calcMagnitude = (x, y, z) => {
    return Math.sqrt(x * x + y * y + z * z) / 100.0; // centi-µT to µT
  };

  // Calculate gradient between sensor 0 and sensor 1
  const calcGradient = () => {
    const dx = (telemetry.fieldX[0] - telemetry.fieldX[1]) / 100.0;
    const dy = (telemetry.fieldY[0] - telemetry.fieldY[1]) / 100.0;
    const dz = (telemetry.fieldZ[0] - telemetry.fieldZ[1]) / 100.0;
    return { dx: (dx / 15.0).toFixed(2), dy: (dy / 15.0).toFixed(2), dz: (dz / 15.0).toFixed(2) };
  };

  const grad = calcGradient();

  // Simple ASCII waveform rendering
  const renderWaveform = (values, color) => {
    if (values.length < 2) return null;
    const max = Math.max(...values.map(Math.abs), 1);
    const bars = values.slice(-40).map(v => {
      const normalized = Math.abs(v) / max;
      const barLength = Math.round(normalized * 20);
      return '█'.repeat(barLength).padEnd(20, ' ');
    });
    return bars.join('\n');
  };

  const mag0History = fieldHistory.map(r => calcMagnitude(r.x0, r.y0, r.z0));
  const mag1History = fieldHistory.map(r => calcMagnitude(r.x1, r.y1, r.z1));
  const mag2History = fieldHistory.map(r => calcMagnitude(r.x2, r.y2, r.z2));

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Sense Mode</Text>
      <Text style={styles.subtitle}>Passive magnetic field monitoring & side-channel analysis</Text>

      {/* Configuration */}
      <View style={styles.inputRow}>
        <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
          <Text style={styles.label}>Rate (Hz)</Text>
          <TextInput style={styles.input} value={rateHz} onChangeText={setRateHz}
            keyboardType="numeric" placeholder="75, 150, 300, 600" />
        </View>
        <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
          <Text style={styles.label}>Duration (s)</Text>
          <TextInput style={styles.input} value={durationS} onChangeText={setDurationS}
            keyboardType="numeric" placeholder="1-3600" />
        </View>
      </View>

      {/* Action buttons */}
      {!isStreaming ? (
        <TouchableOpacity style={styles.startButton} onPress={handleStartStream}>
          <Text style={styles.startButtonText}>▶ START SENSING</Text>
        </TouchableOpacity>
      ) : (
        <TouchableOpacity style={styles.stopButton} onPress={handleStopStream}>
          <Text style={styles.stopButtonText}>■ STOP SENSING</Text>
        </TouchableOpacity>
      )}

      {/* Real-time readings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Real-time Field Readings</Text>
        {[0, 1, 2].map(i => {
          const mag = calcMagnitude(
            telemetry.fieldX[i], telemetry.fieldY[i], telemetry.fieldZ[i]
          );
          return (
            <View key={i} style={styles.sensorCard}>
              <Text style={styles.sensorName}>Sensor {i} ({i * 15}mm)</Text>
              <Text style={styles.sensorMag}>{mag.toFixed(2)} µT</Text>
              <Text style={styles.sensorComponents}>
                X: {(telemetry.fieldX[i] / 100).toFixed(2)}  Y: {(telemetry.fieldY[i] / 100).toFixed(2)}  Z: {(telemetry.fieldZ[i] / 100).toFixed(2)} µT
              </Text>
            </View>
          );
        })}
      </View>

      {/* Gradient analysis */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Gradient (S0 → S1, per mm)</Text>
        <Text style={styles.gradientText}>
          dX/d: {grad.dx} µT/mm{'\n'}
          dY/d: {grad.dy} µT/mm{'\n'}
          dZ/d: {grad.dz} µT/mm
        </Text>
      </View>

      {/* Waveform display */}
      {isStreaming && fieldHistory.length > 1 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Field Magnitude Waveform</Text>
          <View style={styles.waveformBox}>
            <Text style={styles.waveformLabel}>S0 (0mm):</Text>
            <Text style={[styles.waveformText, { color: '#e74c3c' }]}>
              {renderWaveform(mag0History) || 'Collecting...'}
            </Text>
            <Text style={styles.waveformLabel}>S1 (15mm):</Text>
            <Text style={[styles.waveformText, { color: '#2ecc71' }]}>
              {renderWaveform(mag1History) || 'Collecting...'}
            </Text>
            <Text style={styles.waveformLabel}>S2 (30mm):</Text>
            <Text style={[styles.waveformText, { color: '#3498db' }]}>
              {renderWaveform(mag2History) || 'Collecting...'}
            </Text>
          </View>
        </View>
      )}

      {/* Data points collected */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Session Info</Text>
        <Text style={styles.infoText}>Data points: {fieldHistory.length}</Text>
        <Text style={styles.infoText}>Status: {isStreaming ? 'Streaming' : 'Idle'}</Text>
      </View>

      <View style={styles.warningBox}>
        <Text style={styles.warningText}>
          ℹ Sense mode is passive — no field is generated.{'\n'}
          ℹ Useful for detecting current flow through enclosures.{'\n'}
          ℹ Data is logged for post-engagement analysis.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#e74c3c', marginBottom: 4 },
  subtitle: { color: '#95a5a6', fontSize: 12, marginBottom: 16 },
  inputRow: { flexDirection: 'row' },
  inputGroup: { marginBottom: 16 },
  label: { color: '#ecf0f1', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  input: { backgroundColor: '#16213e', color: '#ffffff', borderWidth: 1,
           borderColor: '#34495e', borderRadius: 8, padding: 12, fontSize: 16 },
  startButton: { backgroundColor: '#27ae60', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  startButtonText: { color: '#ffffff', fontSize: 18, fontWeight: 'bold' },
  stopButton: { backgroundColor: '#c0392b', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  stopButtonText: { color: '#ffffff', fontSize: 18, fontWeight: 'bold' },
  section: { backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 12 },
  sectionTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  sensorCard: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 8, marginBottom: 8 },
  sensorName: { color: '#95a5a6', fontSize: 12 },
  sensorMag: { color: '#2ecc71', fontSize: 20, fontWeight: 'bold', fontFamily: 'monospace' },
  sensorComponents: { color: '#ecf0f1', fontSize: 11, fontFamily: 'monospace', marginTop: 4 },
  gradientText: { color: '#ecf0f1', fontSize: 13, fontFamily: 'monospace', lineHeight: 20 },
  waveformBox: { backgroundColor: '#0a0a16', borderRadius: 6, padding: 8 },
  waveformLabel: { color: '#95a5a6', fontSize: 10, marginTop: 4 },
  waveformText: { fontSize: 8, fontFamily: 'monospace', lineHeight: 10 },
  infoText: { color: '#ecf0f1', fontSize: 13, marginBottom: 4 },
  warningBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 16, borderWidth: 1, borderColor: '#3498db' },
  warningText: { color: '#3498db', fontSize: 11, lineHeight: 18 },
});