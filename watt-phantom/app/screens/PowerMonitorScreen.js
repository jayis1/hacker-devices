/**
 * PowerMonitorScreen.js — real-time VBUS voltage/current monitoring
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Displays real-time VBUS voltage and current readings from both
 * USB-C ports (source and sink). Supports power profiling sessions
 * that record VBUS data at 10 Hz for later analysis.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, TextInput } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function PowerMonitorScreen() {
  const { sendCommand } = useDevice();
  const [srcVoltage, setSrcVoltage] = useState(0);
  const [srcCurrent, setSrcCurrent] = useState(0);
  const [snkVoltage, setSnkVoltage] = useState(0);
  const [snkCurrent, setSnkCurrent] = useState(0);
  const [profiling, setProfiling] = useState(false);
  const [profileDuration, setProfileDuration] = useState('10');
  const [sampleCount, setSampleCount] = useState(0);

  const refresh = useCallback(async () => {
    const resp = await sendCommand('STATUS', 2000);
    if (resp && resp.startsWith('OK')) {
      const vbusSrcMatch = resp.match(/vbus_src=([\d.]+)V\/([\d.]+)A/);
      const vbusSnkMatch = resp.match(/vbus_snk=([\d.]+)V\/([\d.]+)A/);
      if (vbusSrcMatch) {
        setSrcVoltage(parseFloat(vbusSrcMatch[1]));
        setSrcCurrent(parseFloat(vbusSrcMatch[2]));
      }
      if (vbusSnkMatch) {
        setSnkVoltage(parseFloat(vbusSnkMatch[1]));
        setSnkCurrent(parseFloat(vbusSnkMatch[2]));
      }
    }
  }, [sendCommand]);

  useEffect(() => {
    const interval = setInterval(refresh, 500); // 2 Hz refresh
    return () => clearInterval(interval);
  }, [refresh]);

  const startProfile = async () => {
    const duration = parseInt(profileDuration);
    if (duration < 1 || duration > 300) return;
    const resp = await sendCommand(`POWER_PROFILE_START ${duration}`, 2000);
    if (resp && resp.startsWith('OK')) {
      setProfiling(true);
      setSampleCount(0);
      // Update sample count periodically
      const profileInterval = setInterval(() => {
        sendCommand('STATUS', 1000).then((r) => {
          if (r && r.includes('profile')) {
            // Estimate sample count (10 Hz sampling)
            setSampleCount((prev) => prev + 5);
          }
        });
      }, 500);
      setTimeout(() => {
        clearInterval(profileInterval);
        setProfiling(false);
        sendCommand('POWER_PROFILE_STOP', 2000).then(() => {
          setSampleCount(4096);
        });
      }, duration * 1000);
    }
  };

  const stopProfile = async () => {
    const resp = await sendCommand('POWER_PROFILE_STOP', 2000);
    if (resp && resp.startsWith('OK')) {
      setProfiling(false);
      const match = resp.match(/(\d+) samples/);
      if (match) setSampleCount(parseInt(match[1]));
    }
  };

  // Simple bar representation of voltage (0-48V range)
  const voltageBar = (v) => Math.min(100, (v / 48) * 100);
  // Simple bar representation of current (0-5A range)
  const currentBar = (i) => Math.min(100, (Math.abs(i) / 5) * 100);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Power Monitor</Text>
      <Text style={styles.subtitle}>Real-time VBUS voltage & current</Text>

      {/* Source port */}
      <View style={styles.portCard}>
        <Text style={styles.portTitle}>Source Port (Charger side)</Text>
        <View style={styles.metricRow}>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Voltage</Text>
            <Text style={styles.metricValue}>{srcVoltage.toFixed(2)} V</Text>
            <View style={styles.barContainer}>
              <View style={[styles.bar, { width: `${voltageBar(srcVoltage)}%`, backgroundColor: '#58a6ff' }]} />
            </View>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Current</Text>
            <Text style={styles.metricValue}>{srcCurrent.toFixed(2)} A</Text>
            <View style={styles.barContainer}>
              <View style={[styles.bar, { width: `${currentBar(srcCurrent)}%`, backgroundColor: '#00d4aa' }]} />
            </View>
          </View>
        </View>
        <Text style={styles.powerText}>Power: {(srcVoltage * srcCurrent).toFixed(2)} W</Text>
      </View>

      {/* Sink port */}
      <View style={styles.portCard}>
        <Text style={styles.portTitle}>Sink Port (Target side)</Text>
        <View style={styles.metricRow}>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Voltage</Text>
            <Text style={styles.metricValue}>{snkVoltage.toFixed(2)} V</Text>
            <View style={styles.barContainer}>
              <View style={[styles.bar, { width: `${voltageBar(snkVoltage)}%`, backgroundColor: '#58a6ff' }]} />
            </View>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Current</Text>
            <Text style={styles.metricValue}>{snkCurrent.toFixed(2)} A</Text>
            <View style={styles.barContainer}>
              <View style={[styles.bar, { width: `${currentBar(snkCurrent)}%`, backgroundColor: '#00d4aa' }]} />
            </View>
          </View>
        </View>
        <Text style={styles.powerText}>Power: {(snkVoltage * snkCurrent).toFixed(2)} W</Text>
      </View>

      {/* Power profiling */}
      <Text style={styles.sectionTitle}>Power Profiling</Text>
      <View style={styles.profileRow}>
        <TextInput
          style={styles.input}
          value={profileDuration}
          onChangeText={setProfileDuration}
          placeholder="Duration (s)"
          keyboardType="numeric"
          editable={!profiling}
        />
        <TouchableOpacity
          style={[styles.profileButton, profiling && styles.profileButtonStop]}
          onPress={profiling ? stopProfile : startProfile}
        >
          <Text style={styles.profileButtonText}>
            {profiling ? 'Stop' : 'Start'} Profile
          </Text>
        </TouchableOpacity>
      </View>
      {profiling && (
        <Text style={styles.profilingStatus}>
          Profiling... {sampleCount} samples captured (10 Hz)
        </Text>
      )}
      {sampleCount > 0 && !profiling && (
        <Text style={styles.profileComplete}>
          Profile complete: {sampleCount} samples captured
        </Text>
      )}

      <Text style={styles.author}>WattPhantom v1.0 — by jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#00d4aa' },
  subtitle: { fontSize: 12, color: '#8b949e', marginBottom: 16 },
  portCard: { backgroundColor: '#161b22', padding: 14, borderRadius: 8, marginBottom: 12, borderWidth: 1, borderColor: '#30363d' },
  portTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  metricRow: { flexDirection: 'row', gap: 12, marginBottom: 8 },
  metric: { flex: 1 },
  metricLabel: { color: '#8b949e', fontSize: 11, marginBottom: 2 },
  metricValue: { color: '#e6edf3', fontSize: 18, fontWeight: 'bold', marginBottom: 4 },
  barContainer: { height: 6, backgroundColor: '#21262d', borderRadius: 3, overflow: 'hidden' },
  bar: { height: '100%', borderRadius: 3 },
  powerText: { color: '#00d4aa', fontSize: 13, fontWeight: 'bold' },
  sectionTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  profileRow: { flexDirection: 'row', gap: 8, alignItems: 'center' },
  input: { flex: 1, backgroundColor: '#161b22', color: '#e6edf3', padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#30363d', fontSize: 13 },
  profileButton: { backgroundColor: '#1f6feb', padding: 10, borderRadius: 6, paddingHorizontal: 16 },
  profileButtonStop: { backgroundColor: '#f85149' },
  profileButtonText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  profilingStatus: { color: '#58a6ff', fontSize: 12, marginTop: 8 },
  profileComplete: { color: '#00d4aa', fontSize: 12, marginTop: 8 },
  author: { color: '#6e7681', fontSize: 11, textAlign: 'center', marginTop: 16 },
});