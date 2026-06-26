/**
 * AttackScreen.js — CPA attack control & live correlation display
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Starts/stops the on-device CPA attack and renders the live per-byte
 * correlation bar chart + recovered key as it converges. The device streams
 * "PROG <n> key=<hex> conv=<n> margin=<f>" lines that we parse here.
 */

import React, { useState, useCallback, useRef, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const BYTE_COLORS = ['#3fb950', '#58a6ff', '#ff6b35', '#f0883e',
  '#bc8cff', '#da3633', '#3fb950', '#58a6ff',
  '#ff6b35', '#f0883e', '#bc8cff', '#da3633',
  '#3fb950', '#58a6ff', '#ff6b35', '#f0883e'];

function parseProgress(line) {
  // Format: "PROG 256 key=00112233445566778899aabbccddeeff conv=4 margin=2.31"
  if (!line.startsWith('PROG')) return null;
  const m = line.match(/PROG\s+(\d+)\s+key=([0-9a-fA-F]+)\s+conv=(\d+)\s+margin=([\d.]+)/);
  if (!m) return null;
  return {
    traces: parseInt(m[1], 10),
    keyHex: m[2].padEnd(32, '0').slice(0, 32),
    converged: parseInt(m[3], 10),
    margin: parseFloat(m[4]),
  };
}

function parseDone(line) {
  // "DONE key=..." or "CONVERGED key=... traces=N" or "STOPPED"
  if (line.startsWith('DONE') || line.startsWith('CONVERGED')) {
    const m = line.match(/key=([0-9a-fA-F]+)/);
    return { done: true, keyHex: m ? m[1].padEnd(32, '0').slice(0, 32) : '', converged: true };
  }
  if (line.startsWith('STOPPED')) return { done: true, stopped: true };
  return null;
}

function KeyDisplay({ keyHex, converged }) {
  if (!keyHex) return <Text style={styles.keyEmpty}>—— —— —— —— —— —— —— —— —— —— —— —— —— —— —— ——</Text>;
  const bytes = keyHex.match(/.{1,2}/g) || [];
  return (
    <View style={styles.keyRow}>
      {bytes.map((b, i) => (
        <View key={i} style={[styles.keyByte,
          { backgroundColor: converged[i] ? '#3fb950' : '#21262d' }]}>
          <Text style={styles.keyByteText}>{b.toUpperCase()}</Text>
        </View>
      ))}
    </View>
  );
}

function CorrelationChart({ keyHex, converged, bestCorr }) {
  const bytes = keyHex ? keyHex.match(/.{1,2}/g) || [] : Array(16).fill('00');
  return (
    <View style={styles.chartBox}>
      <Text style={styles.chartLabel}>Per-byte correlation strength</Text>
      <View style={styles.bars}>
        {bytes.map((b, i) => {
          // Simulated per-byte correlation (real values would be streamed)
          const strength = converged && converged[i] ? 0.85 : 0.2 + (i % 5) * 0.08;
          const h = strength * 100;
          return (
            <View key={i} style={styles.barCol}>
              <View style={[styles.bar, {
                height: h,
                backgroundColor: BYTE_COLORS[i],
                opacity: converged && converged[i] ? 1.0 : 0.4,
              }]} />
              <Text style={styles.barLabel}>{b.toUpperCase()}</Text>
            </View>
          );
        })}
      </View>
    </View>
  );
}

export default function AttackScreen({ navigation }) {
  const { sendCommand, setStreamHandler, attackRunning, setAttackRunning } = useDevice();
  const [keyHex, setKeyHex] = useState('');
  const [convergedArr, setConvergedArr] = useState(Array(16).fill(false));
  const [tracesDone, setTracesDone] = useState(0);
  const [margin, setMargin] = useState(0);
  const [log, setLog] = useState([]);
  const [done, setDone] = useState(false);
  const logRef = useRef([]);

  useEffect(() => {
    setStreamHandler((line) => {
      const prog = parseProgress(line);
      if (prog) {
        setKeyHex(prog.keyHex);
        setTracesDone(prog.traces);
        setMargin(prog.margin);
        // mark first N bytes converged based on conv count
        const conv = Array(16).fill(false);
        for (let i = 0; i < prog.converged; i++) conv[i] = true;
        setConvergedArr(conv);
        logRef.current = [...logRef.current.slice(-30), line];
        setLog([...logRef.current]);
      }
      const fin = parseDone(line);
      if (fin) {
        setDone(true);
        setAttackRunning(false);
        if (fin.keyHex) setKeyHex(fin.keyHex);
        logRef.current = [...logRef.current.slice(-30), line];
        setLog([...logRef.current]);
      }
    });
  }, [setStreamHandler, setAttackRunning]);

  const startAttack = useCallback(async () => {
    setDone(false);
    setKeyHex('');
    setConvergedArr(Array(16).fill(false));
    setTracesDone(0);
    logRef.current = [];
    setLog([]);
    setAttackRunning(true);
    const resp = await sendCommand('ATTACK START', 3000);
    if (resp && resp.startsWith('ACK')) {
      // streaming will populate via the stream handler
    } else {
      Alert.alert('Attack', resp || 'No response');
      setAttackRunning(false);
    }
  }, [sendCommand, setAttackRunning]);

  const stopAttack = useCallback(async () => {
    await sendCommand('ATTACK STOP', 2000);
    setAttackRunning(false);
  }, [sendCommand]);

  const getKey = useCallback(async () => {
    const resp = await sendCommand('KEY', 2000);
    if (resp && resp.startsWith('KEY ')) {
      setKeyHex(resp.slice(4).trim());
    }
  }, [sendCommand]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>🔐 CPA Attack</Text>
      <Text style={styles.hint}>Correlation Power Analysis — recovers AES-128 key on-device.</Text>

      <View style={styles.statusRow}>
        <Text style={[styles.statusBadge, { color: attackRunning ? '#3fb950' : (done ? '#58a6ff' : '#8b949e') }]}>
          {attackRunning ? '● RUNNING' : (done ? '✓ DONE' : '○ IDLE')}
        </Text>
        <Text style={styles.stat}>Traces: {tracesDone}</Text>
        <Text style={styles.stat}>Margin: {margin.toFixed(2)}×</Text>
      </View>

      <Text style={styles.sectionLabel}>Recovered Key (AES-128)</Text>
      <KeyDisplay keyHex={keyHex} converged={convergedArr} />

      <CorrelationChart keyHex={keyHex} converged={convergedArr} bestCorr={null} />

      <View style={styles.btnRow}>
        {!attackRunning ? (
          <TouchableOpacity style={styles.startBtn} onPress={startAttack}>
            <Text style={styles.btnText}>▶ Start Attack</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.stopBtn} onPress={stopAttack}>
            <Text style={styles.btnText}>■ Stop</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={styles.refreshBtn} onPress={getKey}>
          <Text style={styles.btnText}>↻ Get Key</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.sectionLabel}>Live Log</Text>
      <View style={styles.logBox}>
        {log.length === 0 ? (
          <Text style={styles.logEmpty}>No attack data yet.</Text>
        ) : (
          log.map((l, i) => <Text key={i} style={styles.logLine}>{l}</Text>)
        )}
      </View>

      {done && keyHex && (
        <View style={styles.resultBox}>
          <Text style={styles.resultTitle}>✅ Attack Complete</Text>
          <Text style={styles.resultKey}>Key: {keyHex.toUpperCase()}</Text>
          <Text style={styles.resultNote}>Saved to SD card (RESULTS.LOG). Author: jayis1</Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35', marginBottom: 4 },
  hint: { color: '#8b949e', fontSize: 12, marginBottom: 16 },
  statusRow: { flexDirection: 'row', alignItems: 'center', gap: 16, marginBottom: 16 },
  statusBadge: { fontSize: 14, fontWeight: 'bold' },
  stat: { color: '#58a6ff', fontSize: 13, fontFamily: 'monospace' },
  sectionLabel: { color: '#e6edf3', fontSize: 14, fontWeight: 'bold', marginTop: 12, marginBottom: 8 },
  keyRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 4, marginBottom: 8 },
  keyByte: { backgroundColor: '#21262d', borderRadius: 4, padding: 6, minWidth: 28, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  keyByteText: { color: '#e6edf3', fontFamily: 'monospace', fontWeight: 'bold', fontSize: 13 },
  keyEmpty: { color: '#6e7681', fontFamily: 'monospace', fontSize: 14 },
  chartBox: { backgroundColor: '#161b22', borderRadius: 10, padding: 12, marginBottom: 16, borderWidth: 1, borderColor: '#30363d' },
  chartLabel: { color: '#8b949e', fontSize: 12, marginBottom: 8 },
  bars: { flexDirection: 'row', alignItems: 'flex-end', height: 120, gap: 6 },
  barCol: { flex: 1, alignItems: 'center', justifyContent: 'flex-end', height: '100%' },
  bar: { width: '80%', borderRadius: 3, minHeight: 4 },
  barLabel: { color: '#6e7681', fontSize: 8, fontFamily: 'monospace', marginTop: 4 },
  btnRow: { flexDirection: 'row', gap: 12, marginBottom: 16 },
  startBtn: { flex: 1, backgroundColor: '#3fb950', padding: 16, borderRadius: 10, alignItems: 'center' },
  stopBtn: { flex: 1, backgroundColor: '#da3633', padding: 16, borderRadius: 10, alignItems: 'center' },
  refreshBtn: { flex: 1, backgroundColor: '#161b22', padding: 16, borderRadius: 10, alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  logBox: { backgroundColor: '#161b22', borderRadius: 8, padding: 10, maxHeight: 180, borderWidth: 1, borderColor: '#30363d' },
  logLine: { color: '#58a6ff', fontSize: 10, fontFamily: 'monospace', lineHeight: 14 },
  logEmpty: { color: '#6e7681', fontSize: 12 },
  resultBox: { backgroundColor: '#0d2818', borderRadius: 10, padding: 16, marginTop: 16, borderWidth: 1, borderColor: '#3fb950' },
  resultTitle: { color: '#3fb950', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  resultKey: { color: '#e6edf3', fontFamily: 'monospace', fontSize: 16, marginBottom: 8 },
  resultNote: { color: '#8b949e', fontSize: 11 },
});