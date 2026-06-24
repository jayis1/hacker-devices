/*
 * lora-phantom / app / screens / ReplayScreen.js
 * Browse capture buffer, select frames, replay with counter manipulation.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, FlatList } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, hexToBytes } from '../utils/protocol';

export default function ReplayScreen() {
  const { connected, frames, sendCommand } = useDevice();
  const [selectedFrames, setSelectedFrames] = useState(new Set());
  const [counterOverride, setCounterOverride] = useState('');
  const [replayFreq, setReplayFreq] = useState('868.1');
  const [delay, setDelay] = useState('1000');

  const toggleSelect = (idx) => {
    setSelectedFrames(prev => {
      const next = new Set(prev);
      if (next.has(idx)) next.delete(idx);
      else next.add(idx);
      return next;
    });
  };

  const replaySelected = () => {
    const freqHz = Math.round(parseFloat(replayFreq) * 1e6);
    selectedFrames.forEach(idx => {
      const f = frames[idx];
      if (!f) return;
      // Build replay payload: [freq u32][sf u8][bw u8][len u16][frame bytes]
      // We need the raw frame bytes — for the prototype we send the decoded payload
      // In a full app, the raw frame is stored alongside the decoded one
      const payload = new Uint8Array(8);
      payload[0] = freqHz & 0xFF;
      payload[1] = (freqHz >> 8) & 0xFF;
      payload[2] = (freqHz >> 16) & 0xFF;
      payload[3] = (freqHz >> 24) & 0xFF;
      payload[4] = f.sf;
      payload[5] = 0;
      payload[6] = 0;
      payload[7] = 0;
      sendCommand(CMD.REPLAY_SEND, payload);
    });
  };

  const setCounter = () => {
    const fcnt = parseInt(counterOverride, 10);
    if (!isNaN(fcnt)) {
      const p = new Uint8Array(2);
      p[0] = fcnt & 0xFF;
      p[1] = (fcnt >> 8) & 0xFF;
      sendCommand(CMD.REPLAY_SET_FCNT, p);
    }
  };

  const renderItem = ({ item, index }) => {
    const f = item;
    const selected = selectedFrames.has(index);
    return (
      <TouchableOpacity
        style={[styles.frameItem, selected && styles.selectedItem]}
        onPress={() => toggleSelect(index)}>
        <Text style={styles.frameType}>{f.mtypeName} | DevAddr: {f.devAddr}</Text>
        <Text style={styles.frameDetail}>FCnt: {f.fcnt} | FPort: {f.fport}</Text>
        <Text style={styles.framePayload}>Payload: {f.payloadHex.substring(0, 48)}...</Text>
        <Text style={styles.frameMeta}>RSSI: {f.rssi}dBm | {(f.freqHz/1e6).toFixed(1)}MHz | SF{f.sf}</Text>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.header}>🔄 Replay Manager</Text>

      <Text style={styles.section}>Counter Override (ABP replay)</Text>
      <View style={styles.row}>
        <TextInput style={styles.input} value={counterOverride} onChangeText={setCounterOverride}
          placeholder="FCnt (decimal)" placeholderTextColor="#555" keyboardType="number-pad" />
        <TouchableOpacity style={styles.smallBtn} onPress={setCounter}>
          <Text style={styles.smallBtnText}>Set</Text>
        </TouchableOpacity>
      </View>
      <Text style={styles.hint}>Leave empty to replay with original FCnt. Setting a counter
        requires recomputing the MIC (use Injector for that).</Text>

      <View style={styles.row}>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Replay Freq (MHz)</Text>
          <TextInput style={styles.input} value={replayFreq} onChangeText={setReplayFreq}
            keyboardType="decimal-pad" />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Delay (ms)</Text>
          <TextInput style={styles.input} value={delay} onChangeText={setDelay}
            keyboardType="number-pad" />
        </View>
      </View>

      <TouchableOpacity style={styles.replayBtn} onPress={replaySelected} disabled={selectedFrames.size === 0}>
        <Text style={styles.replayBtnText}>
          ⚡ Replay {selectedFrames.size} Frame{selectedFrames.size !== 1 ? 's' : ''}
        </Text>
      </TouchableOpacity>

      <Text style={styles.section}>Captured Frames ({frames.length})</Text>
      {frames.length === 0 ? (
        <Text style={styles.empty}>No frames captured. Run the sniffer first.</Text>
      ) : (
        <FlatList
          data={frames}
          renderItem={renderItem}
          keyExtractor={(item, i) => 'f' + i}
          style={styles.list}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 10 },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 12, marginBottom: 6 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 6 },
  configItem: { flex: 1 },
  configLabel: { color: '#888', fontSize: 11, marginBottom: 3 },
  input: { flex: 1, backgroundColor: '#1a1a1a', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 8, fontSize: 13 },
  smallBtn: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
              borderRadius: 4, padding: 10, paddingHorizontal: 15 },
  smallBtnText: { color: '#00ff88', fontSize: 13 },
  hint: { color: '#666', fontSize: 11, marginBottom: 8 },
  replayBtn: { backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88',
               borderRadius: 6, padding: 14, alignItems: 'center', marginTop: 5, marginBottom: 10 },
  replayBtnText: { color: '#00ff88', fontSize: 15, fontWeight: 'bold' },
  empty: { color: '#888', fontSize: 13, fontStyle: 'italic' },
  list: { flex: 1 },
  frameItem: { backgroundColor: '#1a1a1a', borderRadius: 6, padding: 10, marginBottom: 5,
               borderWidth: 1, borderColor: '#333' },
  selectedItem: { borderColor: '#ffaa00', backgroundColor: '#2a1a0a' },
  frameType: { color: '#fff', fontSize: 13, fontWeight: 'bold' },
  frameDetail: { color: '#aaa', fontSize: 11, marginTop: 2 },
  framePayload: { color: '#888', fontSize: 10, fontFamily: 'monospace', marginTop: 2 },
  frameMeta: { color: '#666', fontSize: 10, marginTop: 2 },
});