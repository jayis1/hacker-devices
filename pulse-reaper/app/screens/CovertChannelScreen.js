/**
 * CovertChannelScreen.js — pair two Pulse-Reapers for air-gap bridging
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Configures two Pulse-Reapers on the same cable pair as a TX/RX pair.
 * The TX device modulates a low-amplitude signal onto the pair through
 * the jacket; the RX device recovers it. This demonstrates that
 * "air-gapped" rooms sharing a cable plant are not truly air-gapped.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function CovertChannelScreen() {
  const { enterMode, sendCommand, injectFrame, MODE, status } = useDevice();
  const [role, setRole] = useState('tx');  // 'tx' or 'rx'
  const [message, setMessage] = useState('Hello from jayis1');
  const [received, setReceived] = useState('');
  const [throughput, setThroughput] = useState(0);
  const [active, setActive] = useState(false);

  useEffect(() => {
    // Poll the RX byte counter if in rx role
    if (active && role === 'rx') {
      const interval = setInterval(async () => {
        // Query the firmware for the covert RX byte count
        // (simplified: we read from the status string)
        if (status && status.startsWith('RX=')) {
          const n = parseInt(status.slice(3), 10);
          setThroughput(n);
        }
      }, 1000);
      return () => clearInterval(interval);
    }
  }, [active, role, status]);

  const handleStart = async () => {
    setActive(true);
    await enterMode(MODE.COVERT);
    if (role === 'tx') {
      // Send each byte of the message as a covert TX byte
      const bytes = Array.from(message, (c) => c.charCodeAt(0));
      for (const b of bytes) {
        await sendCommand(Buffer.from([0x05, b]), 200);  // COVERT_TX opcode (reuse)
      }
    }
  };

  const handleStop = async () => {
    setActive(false);
    await enterMode(MODE.IDLE);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Covert Channel</Text>
      <Text style={styles.desc}>
        Pair two Pulse-Reapers on the same cable pair. The TX device
        modulates a low-amplitude signal through the jacket; the RX device
        recovers it. This demonstrates air-gap bridging via shared copper.
      </Text>

      <View style={styles.roleSelector}>
        <TouchableOpacity
          style={[styles.roleButton, role === 'tx' && styles.roleActive]}
          onPress={() => setRole('tx')}
        >
          <Text style={styles.roleText}>TX (transmit)</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.roleButton, role === 'rx' && styles.roleActive]}
          onPress={() => setRole('rx')}
        >
          <Text style={styles.roleText}>RX (receive)</Text>
        </TouchableOpacity>
      </View>

      {role === 'tx' && (
        <View style={styles.section}>
          <Text style={styles.label}>Message to send</Text>
          <TextInput
            style={styles.input}
            value={message}
            onChangeText={setMessage}
            placeholder="enter message"
            placeholderTextColor="#6e7681"
          />
        </View>
      )}

      {role === 'rx' && (
        <View style={styles.section}>
          <Text style={styles.label}>Received bytes: {throughput}</Text>
          <Text style={styles.receivedBox}>{received || '(waiting...)'}</Text>
        </View>
      )}

      <View style={styles.actions}>
        <TouchableOpacity
          style={[styles.actionButton, active ? styles.stopButton : styles.startButton]}
          onPress={active ? handleStop : handleStart}
        >
          <Text style={styles.actionButtonText}>
            {active ? '⏹ Stop' : '▶ Start Covert Channel'}
          </Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.hint}>
        Note: covert channel bandwidth is low (~10 bytes/s) due to the
        jacket-coupling SNR. For higher rates, use two clamps on the
        same pair at a shorter distance.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0d1117' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#d29922', marginBottom: 4 },
  desc: { color: '#8b949e', fontSize: 12, marginBottom: 16 },
  roleSelector: { flexDirection: 'row', gap: 10, marginBottom: 16 },
  roleButton: { flex: 1, padding: 14, borderRadius: 8, backgroundColor: '#21262d', alignItems: 'center', borderWidth: 1, borderColor: '#30363d' },
  roleActive: { backgroundColor: '#d29922', borderColor: '#d29922' },
  roleText: { color: '#f0f6fc', fontWeight: 'bold' },
  section: { marginBottom: 16 },
  label: { color: '#8b949e', fontSize: 12, marginBottom: 6 },
  input: { backgroundColor: '#161b22', color: '#f0f6fc', borderRadius: 6, paddingHorizontal: 10, height: 40, borderWidth: 1, borderColor: '#30363d' },
  receivedBox: { backgroundColor: '#161b22', color: '#00d4aa', fontFamily: 'monospace', padding: 12, borderRadius: 6, minHeight: 60, borderWidth: 1, borderColor: '#30363d' },
  actions: { marginBottom: 16 },
  actionButton: { padding: 14, borderRadius: 8, alignItems: 'center' },
  startButton: { backgroundColor: '#d29922' },
  stopButton: { backgroundColor: '#f85149' },
  actionButtonText: { color: '#0d1117', fontWeight: 'bold', fontSize: 16 },
  hint: { color: '#6e7681', fontSize: 11, textAlign: 'center' },
});