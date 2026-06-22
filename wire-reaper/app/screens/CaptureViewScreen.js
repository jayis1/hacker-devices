/**
 * CaptureViewScreen.js — Real-time decoded bus traffic display
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, parseCaptureRecords, formatRecord, buildCaptureStart } from '../utils/protocol';
import { Buffer } from 'buffer';

const BUS_COLORS = {
  0: '#4ecca3',  // I2C - green
  1: '#e94560',  // SPI - red
  2: '#7ec8e3',  // UART - blue
  3: '#f0a500',  // 1-Wire - orange
};

export default function CaptureViewScreen() {
  const { sendCommand, setStreamHandler, isCapturing, setIsCapturing } = useDevice();
  const [records, setRecords] = useState([]);
  const [channelFilter, setChannelFilter] = useState(-1); // -1 = all
  const scrollRef = useRef(null);

  // Stream handler: decode incoming data and append to records
  const handleStreamData = useCallback((data) => {
    const newRecs = parseCaptureRecords(data);
    if (newRecs.length > 0) {
      setRecords(prev => [...prev, ...newRecs].slice(-500)); // Keep last 500
    }
  }, []);

  useEffect(() => {
    setStreamHandler(handleStreamData);
    return () => setStreamHandler(null);
  }, [handleStreamData, setStreamHandler]);

  const startCapture = async () => {
    setRecords([]);
    // Start capturing on all channels
    await sendCommand(CMD.CAPTURE_START, buildCaptureStart(0xFFFFFFFF));
    await sendCommand(CMD.CAPTURE_STREAM);
    setIsCapturing(true);
  };

  const stopCapture = async () => {
    await sendCommand(CMD.CAPTURE_STOP);
    setIsCapturing(false);
  };

  const clearRecords = () => {
    setRecords([]);
  };

  const filteredRecords = channelFilter === -1
    ? records
    : records.filter(r => r.channel === channelFilter || r.bus === channelFilter);

  const renderItem = ({ item, index }) => {
    const color = BUS_COLORS[item.bus] || '#888';
    return (
      <View style={[styles.recordRow, { borderLeftColor: color }]}>
        <Text style={[styles.recordText, { color }]}>{formatRecord(item)}</Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.controls}>
        {!isCapturing ? (
          <TouchableOpacity style={[styles.button, styles.startButton]} onPress={startCapture}>
            <Text style={styles.buttonText}>Start Capture</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={[styles.button, styles.stopButton]} onPress={stopCapture}>
            <Text style={styles.buttonText}>Stop Capture</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={[styles.button, styles.clearButton]} onPress={clearRecords}>
          <Text style={styles.buttonText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.filterRow}>
        <TouchableOpacity
          style={[styles.filterChip, channelFilter === -1 && styles.filterActive]}
          onPress={() => setChannelFilter(-1)}
        >
          <Text style={styles.filterText}>ALL</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.filterChip, channelFilter === 0 && styles.filterActive]}
          onPress={() => setChannelFilter(0)}
        >
          <Text style={styles.filterText}>I2C</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.filterChip, channelFilter === 1 && styles.filterActive]}
          onPress={() => setChannelFilter(1)}
        >
          <Text style={styles.filterText}>SPI</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.filterChip, channelFilter === 2 && styles.filterActive]}
          onPress={() => setChannelFilter(2)}
        >
          <Text style={styles.filterText}>UART</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.statsRow}>
        <Text style={styles.statsText}>Records: {filteredRecords.length}</Text>
        <Text style={styles.statsText}>{isCapturing ? '● CAPTURING' : '○ STOPPED'}</Text>
      </View>

      <FlatList
        ref={scrollRef}
        data={filteredRecords}
        keyExtractor={(item, index) => index.toString()}
        renderItem={renderItem}
        style={styles.list}
        onContentSizeChange={() => {
          if (scrollRef.current && filteredRecords.length > 0) {
            scrollRef.current.scrollToEnd({ animated: true });
          }
        }}
        ListEmptyComponent={
          <Text style={styles.empty}>No capture data. Press "Start Capture" to begin.</Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  controls: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  button: { padding: 12, borderRadius: 8, flex: 1, alignItems: 'center' },
  startButton: { backgroundColor: '#4ecca3' },
  stopButton: { backgroundColor: '#e94560' },
  clearButton: { backgroundColor: '#333' },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  filterRow: { flexDirection: 'row', gap: 6, marginBottom: 8 },
  filterChip: { padding: 6, borderRadius: 6, backgroundColor: '#1a1a2e' },
  filterActive: { backgroundColor: '#e94560' },
  filterText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
  statsRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  statsText: { color: '#888', fontSize: 12 },
  list: { flex: 1 },
  recordRow: {
    backgroundColor: '#1a1a2e',
    padding: 8,
    marginBottom: 4,
    borderRadius: 4,
    borderLeftWidth: 3,
  },
  recordText: { fontSize: 11, fontFamily: 'monospace' },
  empty: { color: '#666', textAlign: 'center', padding: 40 },
});