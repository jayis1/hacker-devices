/**
 * FuzzerScreen.js — Autonomous bus fuzzing configuration and monitoring
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, BUS, buildFuzzStart, parseStatus } from '../utils/protocol';
import { Buffer } from 'buffer';

export default function FuzzerScreen() {
  const { sendCommand, setStreamHandler } = useDevice();
  const [busType, setBusType] = useState(BUS.I2C);
  const [channel, setChannel] = useState('0');
  const [isFuzzing, setIsFuzzing] = useState(false);
  const [iterationCount, setIterationCount] = useState(0);
  const [crashCount, setCrashCount] = useState(0);
  const [recentAttempts, setRecentAttempts] = useState([]);
  const [seed, setSeed] = useState('DEADBEEF');
  const intervalRef = useRef(null);

  // Poll status during fuzzing
  useEffect(() => {
    if (isFuzzing) {
      intervalRef.current = setInterval(async () => {
        const resp = await sendCommand(CMD.STATUS);
        if (resp && resp.length >= 20) {
          const st = parseStatus(resp);
          if (st) {
            setIterationCount(st.fuzzIterations);
            if (!st.fuzzActive) {
              setIsFuzzing(false);
            }
          }
        }
      }, 1000);
    } else {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    }
    return () => {
      if (intervalRef.current) clearInterval(intervalRef.current);
    };
  }, [isFuzzing, sendCommand]);

  // Stream handler to track recent attempts
  const handleStreamData = useCallback((data) => {
    // Decode capture records from the fuzz output
    if (data.length >= 16) {
      const bus = data[0];
      const ch = data[1];
      const rw = data[2];
      const flags = data[3];
      const addr = data.readUInt16LE(4);
      const reg = data.readUInt16LE(6);
      const dataLen = data.readUInt16LE(12);
      const dataBytes = [];
      for (let i = 14; i < 14 + Math.min(6, dataLen); i++) {
        dataBytes.push(data[i].toString(16).padStart(2, '0'));
      }
      const busName = ['I2C', 'SPI', 'UART', '1WIRE'][bus] || '?';
      const result = flags & 0x01 ? 'ACK' : flags & 0x02 ? 'NACK' : 'ERR';
      const attempt = `${busName} ch${ch} ${rw ? 'WR' : 'RD'} 0x${addr.toString(16)} reg=0x${reg.toString(16)} [${dataBytes.join(' ')}] → ${result}`;

      setRecentAttempts(prev => [attempt, ...prev].slice(0, 20));

      // Count "crashes" (unexpected responses or timeouts)
      if (result === 'ERR') {
        setCrashCount(prev => prev + 1);
      }
    }
  }, []);

  useEffect(() => {
    setStreamHandler(handleStreamData);
    return () => setStreamHandler(null);
  }, [handleStreamData, setStreamHandler]);

  const handleStartFuzz = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    setIterationCount(0);
    setCrashCount(0);
    setRecentAttempts([]);
    const resp = await sendCommand(CMD.FUZZ_START, buildFuzzStart(busType, ch));
    if (resp && resp[1] === 0) {
      setIsFuzzing(true);
    }
  }, [busType, channel, sendCommand]);

  const handleStopFuzz = useCallback(async () => {
    await sendCommand(CMD.FUZZ_STOP);
    setIsFuzzing(false);
  }, [sendCommand]);

  const busOptions = [
    { type: BUS.I2C, label: 'I²C', maxCh: 2 },
    { type: BUS.SPI, label: 'SPI', maxCh: 2 },
    { type: BUS.UART, label: 'UART', maxCh: 4 },
  ];

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Bus Fuzzer</Text>
      <Text style={styles.subtitle}>Autonomous mutation-based fuzzing of embedded bus interfaces</Text>

      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠️ Fuzzing can crash or damage target devices. Use only on systems
          you own or have explicit authorization to test.
        </Text>
      </View>

      {/* Bus type selection */}
      <Text style={styles.sectionTitle}>Target Bus</Text>
      <View style={styles.busRow}>
        {busOptions.map(opt => (
          <TouchableOpacity
            key={opt.type}
            style={[styles.busChip, busType === opt.type && styles.busActive]}
            onPress={() => { setBusType(opt.type); setChannel('0'); }}
          >
            <Text style={[styles.busText, busType === opt.type && styles.busTextActive]}>
              {opt.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Channel selection */}
      <Text style={styles.sectionTitle}>Channel</Text>
      <TextInput style={styles.input} value={channel} onChangeText={setChannel} keyboardType="numeric" />

      {/* Seed */}
      <Text style={styles.sectionTitle}>RNG Seed (hex)</Text>
      <TextInput style={styles.input} value={seed} onChangeText={setSeed} placeholder="DEADBEEF" />

      {/* Stats */}
      <View style={styles.statsRow}>
        <View style={styles.statBox}>
          <Text style={styles.statValue}>{iterationCount.toLocaleString()}</Text>
          <Text style={styles.statLabel}>Iterations</Text>
        </View>
        <View style={styles.statBox}>
          <Text style={[styles.statValue, { color: crashCount > 0 ? '#e94560' : '#4ecca3' }]}>
            {crashCount}
          </Text>
          <Text style={styles.statLabel}>Anomalies</Text>
        </View>
        <View style={styles.statBox}>
          <Text style={[styles.statValue, { color: isFuzzing ? '#e94560' : '#888' }]}>
            {isFuzzing ? '●' : '○'}
          </Text>
          <Text style={styles.statLabel}>Status</Text>
        </View>
      </View>

      {/* Start/Stop */}
      {!isFuzzing ? (
        <TouchableOpacity style={[styles.button, styles.startButton]} onPress={handleStartFuzz}>
          <Text style={styles.buttonText}>Start Fuzzing</Text>
        </TouchableOpacity>
      ) : (
        <TouchableOpacity style={[styles.button, styles.stopButton]} onPress={handleStopFuzz}>
          <Text style={styles.buttonText}>Stop Fuzzing</Text>
        </TouchableOpacity>
      )}

      {/* Recent attempts */}
      <Text style={styles.sectionTitle}>Recent Attempts</Text>
      <View style={styles.attemptsList}>
        {recentAttempts.length === 0 ? (
          <Text style={styles.muted}>No fuzzing attempts yet. Start fuzzing to see results.</Text>
        ) : (
          recentAttempts.map((attempt, i) => (
            <View key={i} style={styles.attemptRow}>
              <Text style={styles.attemptText}>{attempt}</Text>
            </View>
          ))
        )}
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { color: '#e94560', fontSize: 22, fontWeight: 'bold', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 12, marginBottom: 12 },
  disclaimer: { backgroundColor: '#2a1a1a', borderRadius: 8, padding: 10, marginBottom: 16 },
  disclaimerText: { color: '#ff6b6b', fontSize: 11, textAlign: 'center' },
  sectionTitle: { color: '#e94560', fontSize: 15, fontWeight: 'bold', marginTop: 12, marginBottom: 6 },
  busRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  busChip: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 10, flex: 1, alignItems: 'center' },
  busActive: { backgroundColor: '#e94560' },
  busText: { color: '#888', fontSize: 14, fontWeight: 'bold' },
  busTextActive: { color: '#fff' },
  input: { backgroundColor: '#1a1a2e', color: '#fff', padding: 10, borderRadius: 6, fontFamily: 'monospace', marginBottom: 8 },
  statsRow: { flexDirection: 'row', gap: 8, marginBottom: 16 },
  statBox: { flex: 1, backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, alignItems: 'center' },
  statValue: { color: '#fff', fontSize: 24, fontWeight: 'bold', fontFamily: 'monospace' },
  statLabel: { color: '#666', fontSize: 11, marginTop: 4 },
  button: { padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  startButton: { backgroundColor: '#e94560' },
  stopButton: { backgroundColor: '#4ecca3' },
  buttonText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  attemptsList: { backgroundColor: '#0a0a14', borderRadius: 8, padding: 8 },
  attemptRow: { paddingVertical: 3, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  attemptText: { color: '#4ecca3', fontFamily: 'monospace', fontSize: 11 },
  muted: { color: '#666', textAlign: 'center', padding: 20 },
});