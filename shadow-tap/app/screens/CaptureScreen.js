/**
 * CaptureScreen.js — ShadowTap packet capture control
 * Start/stop PCAP capture, stream capture data, manage SD card files.
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Alert, FlatList
} from 'react-native';

export default function CaptureScreen({ connected, bleManager }) {
  const [capturing, setCapturing] = useState(false);
  const [captures, setCaptures] = useState([]);
  const [streaming, setStreaming] = useState(false);
  const [stats, setStats] = useState({
    packets: 0,
    drops: 0,
    bytes: 0,
  });

  const startCapture = async () => {
    if (!connected) return;
    try {
      await bleManager.startCapture();
      setCapturing(true);
      // Start polling capture stats
      const interval = setInterval(async () => {
        const s = await bleManager.getCaptureStats();
        if (s) setStats(s);
      }, 1000);
      // Store interval for cleanup
      setCapturing({ interval });
    } catch (e) {
      Alert.alert('Error', 'Failed to start capture: ' + e.message);
    }
  };

  const stopCapture = async () => {
    if (!connected || !capturing) return;
    try {
      const result = await bleManager.stopCapture();
      setCapturing(false);
      if (result) {
        setCaptures((prev) => [
          ...prev,
          {
            id: Date.now(),
            filename: result.filename || 'capture.pcap',
            size: result.size || 0,
            packets: stats.packets,
            timestamp: new Date().toLocaleString(),
          },
        ]);
      }
    } catch (e) {
      Alert.alert('Error', 'Failed to stop capture: ' + e.message);
    }
  };

  const startStream = async () => {
    if (!connected) return;
    try {
      await bleManager.streamPcap(true);
      setStreaming(true);
    } catch (e) {
      Alert.alert('Error', 'Failed to start stream: ' + e.message);
    }
  };

  const stopStream = async () => {
    try {
      await bleManager.streamPcap(false);
      setStreaming(false);
    } catch (e) {
      setStreaming(false);
    }
  };

  const formatBytes = (bytes) => {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
    return (bytes / 1048576).toFixed(1) + ' MB';
  };

  const renderCapture = ({ item }) => (
    <View style={styles.captureCard}>
      <View style={styles.captureHeader}>
        <Text style={styles.captureName}>📁 {item.filename}</Text>
        <Text style={styles.captureTime}>{item.timestamp}</Text>
      </View>
      <Text style={styles.captureMeta}>
        {formatBytes(item.size)} • {item.packets.toLocaleString()} packets
      </Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Packet Capture</Text>

      {/* Live Stats */}
      <View style={styles.statsContainer}>
        <View style={styles.statBox}>
          <Text style={styles.statValue}>{stats.packets.toLocaleString()}</Text>
          <Text style={styles.statLabel}>Packets</Text>
        </View>
        <View style={styles.statBox}>
          <Text style={styles.statValue}>{formatBytes(stats.bytes)}</Text>
          <Text style={styles.statLabel}>Captured</Text>
        </View>
        <View style={styles.statBox}>
          <Text style={[styles.statValue, stats.drops > 0 && styles.statWarn]}>
            {stats.drops}
          </Text>
          <Text style={styles.statLabel}>Drops</Text>
        </View>
      </View>

      {/* Capture Controls */}
      <View style={styles.controls}>
        <TouchableOpacity
          style={[styles.ctrlButton, capturing ? styles.ctrlStop : styles.ctrlStart]}
          onPress={capturing ? stopCapture : startCapture}
          disabled={!connected}
        >
          <Text style={styles.ctrlText}>
            {capturing ? '⏹ STOP CAPTURE' : '🔴 START CAPTURE'}
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.ctrlButton, streaming ? styles.ctrlStreamActive : styles.ctrlStream]}
          onPress={streaming ? stopStream : startStream}
          disabled={!connected}
        >
          <Text style={styles.ctrlText}>
            {streaming ? '⏹ STOP STREAM' : '📡 STREAM VIA BLE'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Capture indicator */}
      {capturing && (
        <View style={styles.recordingIndicator}>
          <View style={styles.recDot} />
          <Text style={styles.recText}>CAPTURING</Text>
        </View>
      )}

      {/* Previous Captures */}
      <Text style={styles.sectionTitle}>SAVED CAPTURES</Text>
      <FlatList
        data={captures}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderCapture}
        ListEmptyComponent={
          <Text style={styles.emptyText}>No saved captures</Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 16 },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#00ff88',
    marginBottom: 16,
  },
  statsContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 16,
  },
  statBox: {
    backgroundColor: '#1a1a2e',
    borderRadius: 10,
    padding: 14,
    alignItems: 'center',
    minWidth: 90,
  },
  statValue: { color: '#00ff88', fontSize: 18, fontWeight: 'bold' },
  statLabel: { color: '#666', fontSize: 11, marginTop: 4 },
  statWarn: { color: '#ff4444' },
  controls: {
    flexDirection: 'row',
    gap: 12,
    marginBottom: 16,
  },
  ctrlButton: {
    flex: 1,
    borderRadius: 10,
    padding: 16,
    alignItems: 'center',
  },
  ctrlStart: { backgroundColor: '#00aa55' },
  ctrlStop: { backgroundColor: '#aa3333' },
  ctrlStream: { backgroundColor: '#2244aa' },
  ctrlStreamActive: { backgroundColor: '#aa6633' },
  ctrlText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  recordingIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 12,
  },
  recDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    backgroundColor: '#ff3333',
    marginRight: 8,
  },
  recText: {
    color: '#ff3333',
    fontWeight: 'bold',
    fontSize: 14,
  },
  sectionTitle: {
    color: '#666',
    fontSize: 12,
    fontWeight: 'bold',
    letterSpacing: 2,
    marginBottom: 10,
  },
  captureCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
  },
  captureHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  captureName: { color: '#ccc', fontSize: 14 },
  captureTime: { color: '#666', fontSize: 11 },
  captureMeta: { color: '#888', fontSize: 12 },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 20 },
});