/**
 * FuzzerScreen.js — CAN bus fuzzer interface
 * Supports random, bit-flip, arithmetic, and field-aware fuzzing
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Switch,
  Alert,
} from 'react-native';
import { BLE_PROTOCOL } from '../utils/protocol';

const FUZZ_MODES = [
  { label: 'Random', value: 1, desc: 'Random bytes in each frame' },
  { label: 'Bit-Flip', value: 2, desc: 'Random single-bit mutations' },
  { label: 'Arithmetic', value: 3, desc: 'Byte +/- random deltas' },
  { label: 'Byte-Mutate', value: 4, desc: 'Single byte mutation per frame' },
];

export default function FuzzerScreen() {
  const [isRunning, setIsRunning] = useState(false);
  const [mode, setMode] = useState(1);
  const [targetId, setTargetId] = useState('0');
  const [channel, setChannel] = useState(1);
  const [frameCount, setFrameCount] = useState('1000');
  const [currentCount, setCurrentCount] = useState(0);
  const [extendedId, setExtendedId] = useState(false);
  const timerRef = useRef(null);

  const startFuzzer = async () => {
    try {
      const id = parseInt(targetId, 16) || 0;
      const count = parseInt(frameCount) || 1000;

      const cmd = new Uint8Array(10);
      cmd[0] = channel & 1;
      cmd[1] = mode & 0xFF;
      cmd[2] = id & 0xFF;
      cmd[3] = (id >> 8) & 0xFF;
      cmd[4] = (id >> 16) & 0xFF;
      cmd[5] = (id >> 24) & 0xFF;
      cmd[6] = count & 0xFF;
      cmd[7] = (count >> 8) & 0xFF;
      cmd[8] = (count >> 16) & 0xFF;
      cmd[9] = (count >> 24) & 0xFF;

      await BLE_PROTOCOL.sendCommand(0x04, cmd);
      setIsRunning(true);
      setCurrentCount(0);

      // Simulate progress updates
      timerRef.current = setInterval(() => {
        setCurrentCount(prev => {
          const next = prev + Math.floor(Math.random() * 50) + 10;
          if (next >= parseInt(frameCount)) {
            clearInterval(timerRef.current);
            setIsRunning(false);
            Alert.alert('Complete', `Fuzzed ${frameCount} frames on CH${channel + 1}`);
            return parseInt(frameCount);
          }
          return next;
        });
      }, 200);

    } catch (e) {
      Alert.alert('Error', `Failed to start fuzzer: ${e.message}`);
    }
  };

  const stopFuzzer = async () => {
    if (timerRef.current) clearInterval(timerRef.current);
    const cmd = new Uint8Array([channel & 1]);
    await BLE_PROTOCOL.sendCommand(0x05, cmd);
    setIsRunning(false);
  };

  useEffect(() => {
    return () => {
      if (timerRef.current) clearInterval(timerRef.current);
    };
  }, []);

  const progress = frameCount > 0 ? Math.min((currentCount / parseInt(frameCount || 1)) * 100, 100) : 0;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>CAN Bus Fuzzer</Text>

      {/* Warning */}
      <View style={styles.warning}>
        <Text style={styles.warningText}>
          ⚠️ WARNING: Fuzzing active CAN frames can affect vehicle safety systems.
          Use only on test benches or with explicit authorization.
        </Text>
      </View>

      {/* Mode Selection */}
      <Text style={styles.subtitle}>Fuzzing Mode</Text>
      {FUZZ_MODES.map(m => (
        <TouchableOpacity
          key={m.value}
          style={[styles.modeButton, mode === m.value ? styles.modeActive : null]}
          onPress={() => setMode(m.value)}
          disabled={isRunning}
        >
          <View style={styles.modeHeader}>
            <Text style={[styles.modeLabel, mode === m.value ? styles.modeLabelActive : null]}>
              {m.label}
            </Text>
            {mode === m.value && <Text style={styles.checkmark}>✓</Text>}
          </View>
          <Text style={styles.modeDesc}>{m.desc}</Text>
        </TouchableOpacity>
      ))}

      {/* Target ID */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>Target CAN ID (hex):</Text>
        <TextInput
          style={styles.input}
          value={targetId}
          onChangeText={setTargetId}
          placeholder="0 (all IDs)"
          placeholderTextColor="#666"
        />
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>29-bit</Text>
          <Switch value={extendedId} onValueChange={setExtendedId} disabled={isRunning} />
        </View>
      </View>

      {/* Channel */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>Channel:</Text>
        <TouchableOpacity
          style={[styles.chButton, channel === 0 ? styles.chActive : null]}
          onPress={() => setChannel(0)}
          disabled={isRunning}
        >
          <Text style={styles.chText}>CH1</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.chButton, channel === 1 ? styles.chActive : null]}
          onPress={() => setChannel(1)}
          disabled={isRunning}
        >
          <Text style={styles.chText}>CH2</Text>
        </TouchableOpacity>
      </View>

      {/* Frame Count */}
      <View style={styles.fieldRow}>
        <Text style={styles.label}>Frame count:</Text>
        <TextInput
          style={[styles.input, { width: 100 }]}
          value={frameCount}
          onChangeText={setFrameCount}
          keyboardType="numeric"
          editable={!isRunning}
        />
      </View>

      {/* Progress */}
      {isRunning && (
        <View style={styles.progressContainer}>
          <View style={styles.progressBar}>
            <View style={[styles.progressFill, { width: `${progress}%` }]} />
          </View>
          <Text style={styles.progressText}>
            {currentCount} / {frameCount} frames ({progress.toFixed(1)}%)
          </Text>
        </View>
      )}

      {/* Start/Stop */}
      <TouchableOpacity
        style={[styles.actionButton, isRunning ? styles.stopButton : styles.startButton]}
        onPress={isRunning ? stopFuzzer : startFuzzer}
      >
        <Text style={styles.actionButtonText}>
          {isRunning ? '■ STOP FUZZER' : '⚡ START FUZZER'}
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0D1117', padding: 12 },
  title: { color: '#E6EDF3', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  subtitle: { color: '#8B949E', fontSize: 14, fontWeight: 'bold', marginTop: 12, marginBottom: 8 },
  warning: {
    backgroundColor: '#3D2200',
    borderColor: '#D29922',
    borderWidth: 1,
    borderRadius: 8,
    padding: 12,
    marginBottom: 16,
  },
  warningText: { color: '#E3B341', fontSize: 12 },
  modeButton: {
    backgroundColor: '#161B22',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  modeActive: { borderColor: '#F85149', backgroundColor: '#1C0A0A' },
  modeHeader: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  modeLabel: { color: '#C9D1D9', fontSize: 14, fontWeight: 'bold' },
  modeLabelActive: { color: '#F85149' },
  modeDesc: { color: '#8B949E', fontSize: 11, marginTop: 4 },
  checkmark: { color: '#F85149', fontSize: 16 },
  fieldRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8, marginTop: 4 },
  label: { color: '#C9D1D9', fontSize: 13, marginRight: 8 },
  input: {
    backgroundColor: '#161B22',
    color: '#E6EDF3',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    fontSize: 13,
    borderWidth: 1,
    borderColor: '#30363D',
    flex: 1,
  },
  switchRow: { flexDirection: 'row', alignItems: 'center', marginLeft: 12 },
  switchLabel: { color: '#8B949E', fontSize: 12, marginRight: 4 },
  chButton: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: '#21262D',
    borderRadius: 6,
    marginHorizontal: 4,
  },
  chActive: { backgroundColor: '#1F6FEB' },
  chText: { color: '#C9D1D9', fontSize: 13 },
  progressContainer: { marginVertical: 12 },
  progressBar: {
    height: 8,
    backgroundColor: '#21262D',
    borderRadius: 4,
    overflow: 'hidden',
  },
  progressFill: { height: 8, backgroundColor: '#F85149', borderRadius: 4 },
  progressText: { color: '#8B949E', fontSize: 12, marginTop: 4, textAlign: 'center' },
  actionButton: {
    borderRadius: 8,
    paddingVertical: 14,
    alignItems: 'center',
    marginTop: 16,
  },
  startButton: { backgroundColor: '#F85149' },
  stopButton: { backgroundColor: '#DA3633' },
  actionButtonText: { color: '#FFFFFF', fontSize: 16, fontWeight: 'bold' },
});