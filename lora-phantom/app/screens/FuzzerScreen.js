/*
 * lora-phantom / app / screens / FuzzerScreen.js
 * LoRa PHY-layer fuzzer configuration and control.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Picker } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, FUZZ_MODES, buildFuzzPayload } from '../utils/protocol';

export default function FuzzerScreen() {
  const { connected, status, sendCommand } = useDevice();
  const [mode, setMode] = useState(FUZZ_MODES.BAD_HEADER_CRC);
  const [count, setCount] = useState('50');
  const [delay, setDelay] = useState('200');
  const [running, setRunning] = useState(false);
  const [result, setResult] = useState(null);

  const modeNames = {
    [FUZZ_MODES.BAD_HEADER_CRC]: 'Bad Header CRC',
    [FUZZ_MODES.INVALID_CR]: 'Invalid Coding Rate',
    [FUZZ_MODES.PHANTOM_HEADER]: 'Phantom Header',
    [FUZZ_MODES.IMPLICIT_MISMATCH]: 'Implicit/Explicit Mismatch',
    [FUZZ_MODES.OVERSIZED_PAYLOAD]: 'Oversized Payload',
    [FUZZ_MODES.RANDOM_BYTES]: 'Random Bytes',
  };

  const startFuzz = () => {
    const countVal = parseInt(count, 10);
    const delayVal = parseInt(delay, 10);
    const payload = buildFuzzPayload(mode, countVal, delayVal);
    sendCommand(CMD.FUZZ_START, payload);
    setRunning(true);
    setResult('Fuzzing...');
    setTimeout(() => setRunning(false), countVal * delayVal + 2000);
  };

  const stopFuzz = () => {
    sendCommand(CMD.FUZZ_STOP);
    setRunning(false);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>🧪 LoRa PHY Fuzzer</Text>
      <Text style={styles.description}>
        Transmits malformed LoRa physical-layer frames to test end-device radio
        firmware robustness (header parsing, CRC, length validation).
      </Text>

      <Text style={styles.section}>Fuzz Mode</Text>
      <View style={styles.pickerContainer}>
        <Picker
          selectedValue={mode}
          onValueChange={(v) => setMode(v)}
          style={styles.picker}
          dropdownIconColor="#00ff88">
          {Object.entries(modeNames).map(([k, v]) => (
            <Picker.Item key={k} label={v} value={parseInt(k)} color="#aaa" />
          ))}
        </Picker>
      </View>

      <Text style={styles.modeDescription}>
        {mode === FUZZ_MODES.BAD_HEADER_CRC && 'Transmits frames with corrupted header bytes and CRC disabled. Tests header validation logic.'}
        {mode === FUZZ_MODES.INVALID_CR && 'Transmits with invalid coding rate fields (4/9, 4/10). Tests CR field parsing.'}
        {mode === FUZZ_MODES.PHANTOM_HEADER && 'Sends implicit-header frames with mismatched payload length. Tests implicit header length handling.'}
        {mode === FUZZ_MODES.IMPLICIT_MISMATCH && 'Alternates between explicit and implicit header modes randomly. Tests mode transition logic.'}
        {mode === FUZZ_MODES.OVERSIZED_PAYLOAD && 'Sends 255-byte payloads (maximum). Tests buffer overflow vulnerabilities.'}
        {mode === FUZZ_MODES.RANDOM_BYTES && 'Transmits random payloads with random header/CR configurations. Broad fuzzing.'}
      </Text>

      <View style={styles.row}>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Frame Count</Text>
          <TextInput style={styles.input} value={count} onChangeText={setCount}
            keyboardType="number-pad" editable={!running} />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Delay (ms)</Text>
          <TextInput style={styles.input} value={delay} onChangeText={setDelay}
            keyboardType="number-pad" editable={!running} />
        </View>
      </View>

      <Text style={styles.estimate}>
        Estimated duration: {parseInt(count) * parseInt(delay) / 1000}s
      </Text>

      {!running ? (
        <TouchableOpacity style={styles.startBtn} onPress={startFuzz} disabled={!connected}>
          <Text style={styles.btnText}>⚡ Start Fuzzing</Text>
        </TouchableOpacity>
      ) : (
        <TouchableOpacity style={styles.stopBtn} onPress={stopFuzz}>
          <Text style={styles.btnText}>⏹ Stop</Text>
        </TouchableOpacity>
      )}

      {result && (
        <View style={styles.resultBox}>
          <Text style={styles.resultText}>{result}</Text>
        </View>
      )}

      <View style={styles.warning}>
        <Text style={styles.warningText}>
          ⚠️ PHY fuzzing transmits LoRa frames. Ensure regulatory compliance
          and authorization. Observe duty cycle limits. Monitor target device
          for crashes, reboots, or unexpected behavior.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 8 },
  description: { color: '#888', fontSize: 12, marginBottom: 12, lineHeight: 18 },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 12, marginBottom: 6 },
  pickerContainer: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#333',
                     borderRadius: 4, marginBottom: 8 },
  picker: { color: '#ccc' },
  modeDescription: { color: '#aaa', fontSize: 11, marginBottom: 10, lineHeight: 16,
                     backgroundColor: '#1a1a1a', padding: 10, borderRadius: 4 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 6 },
  configItem: { flex: 1 },
  configLabel: { color: '#888', fontSize: 11, marginBottom: 3 },
  input: { backgroundColor: '#1a1a1a', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 8, fontSize: 14 },
  estimate: { color: '#666', fontSize: 11, marginBottom: 8 },
  startBtn: { backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88',
              borderRadius: 6, padding: 14, alignItems: 'center' },
  stopBtn: { backgroundColor: '#3a0a0a', borderWidth: 1, borderColor: '#ff6666',
             borderRadius: 6, padding: 14, alignItems: 'center' },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: 'bold' },
  resultBox: { backgroundColor: '#0a2a1a', borderRadius: 6, padding: 10, marginTop: 10,
               borderWidth: 1, borderColor: '#00ff88' },
  resultText: { color: '#00ff88', fontSize: 13 },
  warning: { backgroundColor: '#2a1a0a', borderWidth: 1, borderColor: '#ff6600',
             borderRadius: 6, padding: 10, marginTop: 15 },
  warningText: { color: '#ffaa44', fontSize: 11 },
});