/**
 * LiveCaptureScreen.js — real-time decoded fieldbus frame viewer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Enters sniff mode, registers a frame callback, and displays decoded
 * frames (Modbus RTU, Profibus DP, HART, CAN, POTS DTMF) in a scrolling
 * list. Supports filtering by address and function code.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, TouchableOpacity, FlatList, TextInput, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { decodeFrame, protocolName } from '../utils/protocol';

export default function LiveCaptureScreen() {
  const { enterMode, stopSniff, setFrameCallback, MODE } = useDevice();
  const [frames, setFrames] = useState([]);
  const [capturing, setCapturing] = useState(false);
  const [addrFilter, setAddrFilter] = useState('');
  const [frameCount, setFrameCount] = useState(0);
  const framesRef = useRef([]);

  useEffect(() => {
    setFrameCallback((rawFrame) => {
      // rawFrame = [proto_id, dst_lo, dst_hi, func, len, data...]
      if (rawFrame.length < 5) return;
      const decoded = decodeFrame(rawFrame);
      if (addrFilter) {
        const a = parseInt(addrFilter, 10);
        if (!isNaN(a) && decoded.dst_addr !== a) return;
      }
      framesRef.current = [decoded, ...framesRef.current].slice(0, 500);
      setFrames(framesRef.current);
      setFrameCount((c) => c + 1);
    });
  }, [setFrameCallback, addrFilter]);

  const handleStart = async () => {
    setCapturing(true);
    framesRef.current = [];
    setFrames([]);
    setFrameCount(0);
    await enterMode(MODE.SNIFF);
  };

  const handleStop = async () => {
    setCapturing(false);
    await stopSniff();
  };

  const renderItem = ({ item, index }) => (
    <View style={styles.frameItem}>
      <View style={styles.frameHeader}>
        <Text style={styles.frameIndex}>#{frameCount - index}</Text>
        <Text style={[styles.frameProto, { color: protocolColor(item.protocol_id) }]}>
          {protocolName(item.protocol_id)}
        </Text>
        <Text style={styles.frameAddr}>addr={item.dst_addr}</Text>
        <Text style={styles.frameFunc}>fn={item.function}</Text>
      </View>
      <Text style={styles.frameData}>{item.summary}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        <TouchableOpacity
          style={[styles.button, capturing ? styles.stopButton : styles.startButton]}
          onPress={capturing ? handleStop : handleStart}
        >
          <Text style={styles.buttonText}>{capturing ? '⏹ Stop' : '▶ Start Sniff'}</Text>
        </TouchableOpacity>
        <Text style={styles.counter}>{frameCount} frames</Text>
      </View>

      <View style={styles.filterBar}>
        <Text style={styles.filterLabel}>Filter addr:</Text>
        <TextInput
          style={styles.filterInput}
          value={addrFilter}
          onChangeText={setAddrFilter}
          placeholder="any"
          placeholderTextColor="#6e7681"
          keyboardType="numeric"
        />
      </View>

      <FlatList
        data={frames}
        keyExtractor={(item, index) => `${index}`}
        renderItem={renderItem}
        ListEmptyComponent={
          <Text style={styles.empty}>No frames yet. Tap "Start Sniff" to begin.</Text>
        }
      />
    </View>
  );
}

function protocolColor(id) {
  const colors = {
    1: '#00d4aa',  // Modbus RTU
    2: '#a371f7',  // Profibus DP
    3: '#d29922',  // HART
    4: '#ff6b35',  // CAN
    5: '#8b949e',  // POTS
  };
  return colors[id] || '#f0f6fc';
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 12, backgroundColor: '#0d1117' },
  toolbar: { flexDirection: 'row', alignItems: 'center', gap: 12, marginBottom: 8 },
  button: { padding: 12, borderRadius: 8, minWidth: 130, alignItems: 'center' },
  startButton: { backgroundColor: '#00d4aa' },
  stopButton: { backgroundColor: '#f85149' },
  buttonText: { color: '#0d1117', fontWeight: 'bold', fontSize: 14 },
  counter: { color: '#8b949e', fontSize: 13 },
  filterBar: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  filterLabel: { color: '#8b949e', fontSize: 12, marginRight: 6 },
  filterInput: { flex: 1, backgroundColor: '#161b22', color: '#f0f6fc', borderRadius: 6, paddingHorizontal: 10, height: 34, borderWidth: 1, borderColor: '#30363d' },
  frameItem: { backgroundColor: '#161b22', padding: 10, borderRadius: 6, marginBottom: 6, borderWidth: 1, borderColor: '#30363d' },
  frameHeader: { flexDirection: 'row', gap: 10, alignItems: 'center', marginBottom: 4 },
  frameIndex: { color: '#6e7681', fontSize: 11 },
  frameProto: { fontSize: 12, fontWeight: 'bold' },
  frameAddr: { color: '#8b949e', fontSize: 11 },
  frameFunc: { color: '#8b949e', fontSize: 11 },
  frameData: { color: '#f0f6fc', fontSize: 12, fontFamily: 'monospace' },
  empty: { color: '#6e7681', textAlign: 'center', marginTop: 40, fontSize: 14 },
});