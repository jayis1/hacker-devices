/**
 * FuzzerScreen.js — PD fuzz campaign configuration and control
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  Picker, Alert, ProgressBarAndroid,
} from 'react-native';
import { CMD, FUZZ_PROFILES, EmberTapConnection } from '../utils/protocol';

export default function FuzzerScreen() {
  const [count, setCount] = useState('1000');
  const [seed, setSeed] = useState('3735928559');  /* 0xDEAD */
  const [profile, setProfile] = useState(FUZZ_PROFILES.HEADER);
  const [running, setRunning] = useState(false);
  const [progress, setProgress] = useState(0);
  const [sent, setSent] = useState(0);
  const [crashes, setCrashes] = useState(0);
  const [conn, setConn] = useState(null);

  const startFuzz = async () => {
    if (!conn) {
      Alert.alert('Not connected', 'Connect from the Dashboard tab first.');
      return;
    }
    const cnt = parseInt(count, 10) || 1000;
    const sd = parseInt(seed, 10) || 0xDEAD;
    /* Set profile first */
    await conn.send(CMD.FUZZ_PROFILE, [profile]);
    /* Encode count + seed */
    const { encodeFuzzStart } = require('../utils/protocol');
    await conn.send(CMD.FUZZ_START, encodeFuzzStart(cnt, sd));
    setRunning(true);
    setProgress(0);
    setSent(0);
    setCrashes(0);
  };

  const stopFuzz = async () => {
    if (conn) await conn.send(CMD.FUZZ_STOP);
    setRunning(false);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>PD Fuzzer</Text>
      <Text style={styles.warning}>
        ⚠ Fuzzing can crash or damage the DUT's PD controller.
        Ensure DUT is authorized test equipment only.
      </Text>

      <View style={styles.field}>
        <Text style={styles.label}>Frame count</Text>
        <TextInput
          style={styles.input}
          value={count}
          onChangeText={setCount}
          keyboardType="numeric"
          editable={!running}
        />
      </View>

      <View style={styles.field}>
        <Text style={styles.label}>Random seed (decimal)</Text>
        <TextInput
          style={styles.input}
          value={seed}
          onChangeText={setSeed}
          keyboardType="numeric"
          editable={!running}
        />
      </View>

      <View style={styles.field}>
        <Text style={styles.label}>Mutation profile</Text>
        <View style={styles.pickerWrap}>
          <Picker
            selectedValue={profile}
            onValueChange={setProfile}
            enabled={!running}
            style={styles.picker}
            dropdownIconColor="#FF6600"
          >
            <Picker.Item label="Header bitfield fuzz" value={FUZZ_PROFILES.HEADER} />
            <Picker.Item label="PDO voltage/current fuzz" value={FUZZ_PROFILES.PDO} />
            <Picker.Item label="Timing violation" value={FUZZ_PROFILES.TIMING} />
            <Picker.Item label="Chunked message fuzz" value={FUZZ_PROFILES.CHUNK} />
          </Picker>
        </View>
      </View>

      {running && (
        <View style={styles.progressSection}>
          <ProgressBarAndroid
            styleAttr="Horizontal"
            color="#FF6600"
            progress={progress}
          />
          <View style={styles.statRow}>
            <Text style={styles.statText}>Sent: {sent}</Text>
            <Text style={[styles.statText, styles.crashText]}>Crashes: {crashes}</Text>
          </View>
        </View>
      )}

      <View style={styles.buttonRow}>
        {!running ? (
          <TouchableOpacity style={styles.startBtn} onPress={startFuzz}>
            <Text style={styles.btnText}>Start Campaign</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.stopBtn} onPress={stopFuzz}>
            <Text style={styles.btnText}>Stop</Text>
          </TouchableOpacity>
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { color: '#FF6600', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  warning: { color: '#FFAA00', fontSize: 11, marginBottom: 20, fontStyle: 'italic' },
  field: { marginBottom: 16 },
  label: { color: '#888', fontSize: 14, marginBottom: 4 },
  input: {
    backgroundColor: '#16213e', color: '#fff', borderRadius: 6,
    padding: 12, fontSize: 16, borderWidth: 1, borderColor: '#333',
  },
  pickerWrap: {
    backgroundColor: '#16213e', borderRadius: 6,
    borderWidth: 1, borderColor: '#333',
  },
  picker: { color: '#fff', height: 50 },
  progressSection: { marginBottom: 20 },
  statRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 8 },
  statText: { color: '#fff', fontSize: 14 },
  crashText: { color: '#FF4444', fontWeight: 'bold' },
  buttonRow: { marginTop: 20 },
  startBtn: {
    backgroundColor: '#FF6600', padding: 16, borderRadius: 8, alignItems: 'center',
  },
  stopBtn: {
    backgroundColor: '#FF4444', padding: 16, borderRadius: 8, alignItems: 'center',
  },
  btnText: { color: '#fff', fontSize: 18, fontWeight: 'bold' },
});

/* end of file — author: jayis1 */