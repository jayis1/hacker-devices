/**
 * InjectScreen.js — Manual SBS/PMBus command injection console.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Lets the operator craft and send arbitrary SMBus read/write/block
 * commands to the battery side, or inject fake host-originated writes
 * onto the host side.  Useful for fuzzing charger ICs and battery
 * management firmware.
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity, ScrollView, Alert,
} from 'react-native';
import { OP, SBS_NAMES } from '../services/BleService';

const PRESETS = [
  { label: 'Read SoC',        addr: '0x0B', cmd: '0x0D', wbytes: '', desc: 'Read RelativeStateOfCharge' },
  { label: 'Read Voltage',    addr: '0x0B', cmd: '0x06', wbytes: '', desc: 'Read cell voltage (mV)' },
  { label: 'Read Temperature',addr: '0x0B', cmd: '0x05', wbytes: '', desc: 'Read pack temp (0.1K)' },
  { label: 'Read Mfr Name',   addr: '0x0B', cmd: '0x20', wbytes: '', desc: 'Read block: ManufacturerName' },
  { label: 'Read Serial',     addr: '0x0B', cmd: '0x1C', wbytes: '', desc: 'Read SerialNumber' },
  { label: 'Write 0V Charge', addr: '0x0B', cmd: '0x15', wbytes: '00,00', desc: 'Set ChargingVoltage=0' },
  { label: 'Write 0A Charge', addr: '0x0B', cmd: '0x14', wbytes: '00,00', desc: 'Set ChargingCurrent=0' },
];

export default function InjectScreen({ sendCommand }) {
  const [addr, setAddr] = useState('0x0B');
  const [cmd, setCmd] = useState('0x0D');
  const [wbytes, setWbytes] = useState('');
  const [log, setLog] = useState([]);

  const parseHex = (s) => {
    const v = parseInt(s.replace('0x', ''), 16);
    return isNaN(v) ? 0 : v;
  };

  const parseBytes = (s) =>
    s.split(',').map((b) => parseInt(b.trim(), 16) & 0xFF).filter((b) => !isNaN(b));

  const send = () => {
    const a = parseHex(addr);
    const c = parseHex(cmd);
    const wb = parseBytes(wbytes);
    sendCommand(OP.INJECT, [a, c, wb.length, ...wb]);
    const entry = `[${new Date().toLocaleTimeString()}] INJECT addr=${addr} cmd=${cmd} ` +
                  (wb.length ? `w=[${wb.map((b) => '0x' + b.toString(16).padStart(2, '0')).join(',')}]` : '(read)');
    setLog((prev) => [entry, ...prev].slice(0, 50));
  };

  const applyPreset = (p) => {
    setAddr(p.addr);
    setCmd(p.cmd);
    setWbytes(p.wbytes);
  };

  const fireGlitch = () => {
    sendCommand(OP.GLITCH, [50, 0]);  // 50 us pulse
    setLog((prev) => [`[${new Date().toLocaleTimeString()}] GLITCH 50us pulse fired`, ...prev].slice(0, 50));
  };

  const cmdName = SBS_NAMES[parseHex(cmd)] || 'Unknown';

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Inject SMBus Command</Text>

        <Text style={styles.label}>Slave Address (7-bit hex)</Text>
        <TextInput style={styles.input} value={addr} onChangeText={setAddr}
          placeholder="0x0B" placeholderTextColor="#555" />

        <Text style={styles.label}>Command Code (hex) — {cmdName}</Text>
        <TextInput style={styles.input} value={cmd} onChangeText={setCmd}
          placeholder="0x0D" placeholderTextColor="#555" />

        <Text style={styles.label}>Write Bytes (comma-sep hex, empty = read)</Text>
        <TextInput style={styles.input} value={wbytes} onChangeText={setWbytes}
          placeholder="64,00" placeholderTextColor="#555" />

        <View style={styles.btnRow}>
          <TouchableOpacity style={styles.sendBtn} onPress={send}>
            <Text style={styles.sendBtnText}>▶ Inject</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.glitchBtn} onPress={fireGlitch}>
            <Text style={styles.glitchBtnText}>⚡ Glitch 50µs</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.title}>Presets</Text>
        {PRESETS.map((p) => (
          <TouchableOpacity key={p.label} style={styles.preset} onPress={() => applyPreset(p)}>
            <View style={{ flex: 1 }}>
              <Text style={styles.presetLabel}>{p.label}</Text>
              <Text style={styles.presetDesc}>{p.desc}</Text>
            </View>
            <Text style={styles.presetMeta}>addr={p.addr} cmd={p.cmd}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.card}>
        <Text style={styles.title}>Injection Log</Text>
        {log.length === 0 ? (
          <Text style={styles.empty}>No injections yet.</Text>
        ) : (
          log.map((entry, i) => (
            <Text key={i} style={styles.logEntry}>{entry}</Text>
          ))
        )}
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  card: { backgroundColor: '#16213e', margin: 10, borderRadius: 10, padding: 15 },
  title: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  label: { color: '#888', fontSize: 12, marginTop: 8, marginBottom: 4 },
  input: {
    backgroundColor: '#1a1a2e', color: '#fff', borderWidth: 1,
    borderColor: '#333', borderRadius: 6, padding: 10, fontSize: 14,
  },
  btnRow: { flexDirection: 'row', marginTop: 14 },
  sendBtn: {
    backgroundColor: '#e94560', borderRadius: 8, padding: 12,
    alignItems: 'center', flex: 1, marginRight: 6,
  },
  sendBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  glitchBtn: {
    borderWidth: 1, borderColor: '#f80', borderRadius: 8, padding: 12,
    alignItems: 'center', flex: 1,
  },
  glitchBtnText: { color: '#f80', fontWeight: 'bold', fontSize: 15 },
  preset: {
    flexDirection: 'row', alignItems: 'center', padding: 10,
    borderBottomWidth: 1, borderBottomColor: '#222',
  },
  presetLabel: { color: '#fff', fontSize: 13, fontWeight: '600' },
  presetDesc: { color: '#888', fontSize: 11, marginTop: 2 },
  presetMeta: { color: '#666', fontSize: 10, marginLeft: 8 },
  empty: { color: '#555', fontSize: 13, fontStyle: 'italic', padding: 10 },
  logEntry: { color: '#0f0', fontSize: 11, fontFamily: 'monospace', paddingVertical: 3 },
});