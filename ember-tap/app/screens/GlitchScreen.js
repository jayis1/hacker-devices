/**
 * GlitchScreen.js — VBUS power glitch control
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity, Alert, Switch,
} from 'react-native';
import { CMD, EmberTapConnection } from '../utils/protocol';

export default function GlitchScreen() {
  const [duration, setDuration] = useState('100');
  const [type, setType] = useState(0);  /* 0=droop, 1=crowbar */
  const [armed, setArmed] = useState(false);
  const [conn, setConn] = useState(null);
  const [lastResult, setLastResult] = useState(null);

  const fireGlitch = async () => {
    if (!conn) {
      Alert.alert('Not connected', 'Connect from Dashboard first.');
      return;
    }
    if (!armed) {
      Alert.alert('Not armed', 'Arm the glitch before firing.');
      return;
    }
    const us = parseInt(duration, 10);
    if (us < 1 || us > 100000) {
      Alert.alert('Invalid duration', '1–100000 µs');
      return;
    }
    const { encodeGlitch } = require('../utils/protocol');
    await conn.send(CMD.VBUS_GLITCH, encodeGlitch(us, type));
    setLastResult({ us, type, time: new Date().toLocaleTimeString() });
    /* Auto-disarm for safety */
    setArmed(false);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>VBUS Glitch</Text>
      <Text style={styles.warning}>
        ⚠ DANGEROUS. Momentarily shorts or droops VBUS to the DUT.
        Can cause brown-out, data corruption, or hardware damage.
        Hardware interlock disables above 20 V.
      </Text>

      <View style={styles.field}>
        <Text style={styles.label}>Duration (µs)</Text>
        <TextInput
          style={styles.input}
          value={duration}
          onChangeText={setDuration}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.typeRow}>
        <TouchableOpacity
          style={[styles.typeBtn, type === 0 && styles.typeBtnActive]}
          onPress={() => setType(0)}
        >
          <Text style={styles.typeText}>Droop</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.typeBtn, type === 1 && styles.typeBtnActiveCrowbar]}
          onPress={() => setType(1)}
        >
          <Text style={styles.typeText}>Crowbar</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.armRow}>
        <Text style={styles.armLabel}>Arm (safety toggle)</Text>
        <Switch
          value={armed}
          onValueChange={setArmed}
          trackColor={{ false: '#333', true: '#FF4444' }}
        />
      </View>

      <TouchableOpacity
        style={[styles.fireBtn, !armed && styles.fireBtnDisabled]}
        onPress={fireGlitch}
        disabled={!armed}
      >
        <Text style={styles.fireText}>🔥 FIRE GLITCH</Text>
      </TouchableOpacity>

      {lastResult && (
        <View style={styles.resultBox}>
          <Text style={styles.resultText}>
            Last: {lastResult.us}µs {lastResult.type === 0 ? 'droop' : 'crowbar'}
            {'\n'}at {lastResult.time}
          </Text>
        </View>
      )}

      <Text style={styles.footer}>jayis1 — authorized research only</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { color: '#FF4444', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  warning: { color: '#FFAA00', fontSize: 11, marginBottom: 24, fontStyle: 'italic' },
  field: { marginBottom: 16 },
  label: { color: '#888', fontSize: 14, marginBottom: 4 },
  input: {
    backgroundColor: '#16213e', color: '#fff', borderRadius: 6,
    padding: 12, fontSize: 16, borderWidth: 1, borderColor: '#333',
  },
  typeRow: { flexDirection: 'row', marginBottom: 20 },
  typeBtn: {
    flex: 1, padding: 12, borderRadius: 6, alignItems: 'center',
    backgroundColor: '#16213e', marginHorizontal: 4, borderWidth: 1, borderColor: '#333',
  },
  typeBtnActive: { borderColor: '#FF6600', backgroundColor: '#2a1a0e' },
  typeBtnActiveCrowbar: { borderColor: '#FF4444', backgroundColor: '#2a0e0e' },
  typeText: { color: '#fff', fontSize: 16 },
  armRow: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    marginBottom: 20, backgroundColor: '#16213e', padding: 12, borderRadius: 6,
  },
  armLabel: { color: '#fff', fontSize: 16 },
  fireBtn: {
    backgroundColor: '#FF4444', padding: 20, borderRadius: 8, alignItems: 'center',
  },
  fireBtnDisabled: { backgroundColor: '#333' },
  fireText: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  resultBox: {
    backgroundColor: '#16213e', padding: 12, borderRadius: 6, marginTop: 20,
  },
  resultText: { color: '#4CAF50', fontSize: 14 },
  footer: { color: '#555', fontSize: 11, textAlign: 'center', marginTop: 16 },
});

/* end of file — author: jayis1 */