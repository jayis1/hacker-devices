/**
 * ReplayManagerScreen.js — Load and replay captured bus transactions
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, parseCaptureRecords, formatRecord } from '../utils/protocol';

export default function ReplayManagerScreen() {
  const { sendCommand } = useDevice();
  const [captures, setCaptures] = useState([]);
  const [selectedCapture, setSelectedCapture] = useState(null);
  const [loopCount, setLoopCount] = useState('1');
  const [delayMs, setDelayMs] = useState('100');
  const [replayStatus, setReplayStatus] = useState('');
  const [isReplaying, setIsReplaying] = useState(false);

  // Simulated capture list (in production, loaded from SD card)
  const loadCaptures = useCallback(async () => {
    // In a full implementation, we'd query the device's SD card for saved captures
    setCaptures([
      { id: 1, name: 'I2C EEPROM Read (0x50)', records: 42, bus: 'I2C', size: '672 B' },
      { id: 2, name: 'SPI Flash Boot Dump', records: 1280, bus: 'SPI', size: '20 KB' },
      { id: 3, name: 'UART Console Session', records: 560, bus: 'UART', size: '9 KB' },
      { id: 4, name: '1-Wire iButton Read', records: 8, bus: '1-WIRE', size: '128 B' },
    ]);
  }, []);

  const handleReplay = useCallback(async () => {
    if (!selectedCapture) {
      setReplayStatus('Select a capture to replay');
      return;
    }
    setIsReplaying(true);
    setReplayStatus(`Replaying "${selectedCapture.name}" ${loopCount}x with ${delayMs}ms delay...`);

    const payload = Buffer.alloc(8);
    payload.writeUInt32LE(selectedCapture.id, 0);
    payload.writeUInt32LE(parseInt(loopCount, 10) || 1, 4);

    const resp = await sendCommand(CMD.REPLAY_RUN, payload);
    if (resp && resp[1] === 0) {
      setReplayStatus(`Replay complete: ${selectedCapture.name}`);
    } else {
      setReplayStatus(`Replay: ${resp && resp[1] === -6 ? 'No capture loaded on device' : 'Failed'}`);
    }
    setIsReplaying(false);
  }, [selectedCapture, loopCount, sendCommand]);

  const renderCapture = ({ item }) => (
    <TouchableOpacity
      style={[styles.captureCard, selectedCapture?.id === item.id && styles.captureSelected]}
      onPress={() => setSelectedCapture(item)}
    >
      <View style={styles.captureHeader}>
        <Text style={styles.captureName}>{item.name}</Text>
        <Text style={styles.captureBus}>{item.bus}</Text>
      </View>
      <View style={styles.captureMeta}>
        <Text style={styles.metaText}>{item.records} records</Text>
        <Text style={styles.metaText}>{item.size}</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Replay Manager</Text>
      <Text style={styles.subtitle}>Load and replay captured bus transactions</Text>

      <TouchableOpacity style={styles.refreshButton} onPress={loadCaptures}>
        <Text style={styles.refreshText}>Refresh Captures</Text>
      </TouchableOpacity>

      <FlatList
        data={captures}
        keyExtractor={item => item.id.toString()}
        renderItem={renderCapture}
        style={styles.list}
        ListEmptyComponent={
          <Text style={styles.empty}>No captures available. Tap "Refresh Captures" to load from device SD card.</Text>
        }
      />

      {selectedCapture && (
        <View style={styles.replayPanel}>
          <Text style={styles.sectionTitle}>Replay Settings</Text>
          <View style={styles.row}>
            <Text style={styles.label}>Loop count</Text>
            <TextInput style={styles.input} value={loopCount} onChangeText={setLoopCount} keyboardType="numeric" />
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Delay (ms)</Text>
            <TextInput style={styles.input} value={delayMs} onChangeText={setDelayMs} keyboardType="numeric" />
          </View>
          <TouchableOpacity
            style={[styles.replayButton, isReplaying && styles.replayingButton]}
            onPress={handleReplay}
            disabled={isReplaying}
          >
            <Text style={styles.replayButtonText}>
              {isReplaying ? 'Replaying...' : 'Start Replay'}
            </Text>
          </TouchableOpacity>
        </View>
      )}

      {replayStatus ? <Text style={styles.status}>{replayStatus}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { color: '#e94560', fontSize: 22, fontWeight: 'bold', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 13, marginBottom: 16 },
  refreshButton: { backgroundColor: '#1a1a2e', padding: 10, borderRadius: 6, alignItems: 'center', marginBottom: 12 },
  refreshText: { color: '#7ec8e3', fontSize: 13 },
  list: { flex: 1 },
  captureCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
    borderWidth: 2,
    borderColor: 'transparent',
  },
  captureSelected: { borderColor: '#e94560' },
  captureHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  captureName: { color: '#fff', fontSize: 15, fontWeight: 'bold', flex: 1 },
  captureBus: { color: '#4ecca3', fontSize: 11, fontFamily: 'monospace' },
  captureMeta: { flexDirection: 'row', gap: 16 },
  metaText: { color: '#666', fontSize: 12 },
  replayPanel: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginTop: 8 },
  sectionTitle: { color: '#e94560', fontSize: 15, fontWeight: 'bold', marginBottom: 8 },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 8, gap: 8 },
  label: { color: '#888', fontSize: 13, width: 100 },
  input: { backgroundColor: '#0a0a14', color: '#fff', padding: 8, borderRadius: 6, flex: 1, fontFamily: 'monospace' },
  replayButton: { backgroundColor: '#e94560', padding: 12, borderRadius: 8, alignItems: 'center', marginTop: 8 },
  replayingButton: { backgroundColor: '#555' },
  replayButtonText: { color: '#fff', fontWeight: 'bold' },
  empty: { color: '#666', textAlign: 'center', padding: 30 },
  status: { color: '#aaa', fontSize: 13, marginTop: 12, textAlign: 'center' },
});