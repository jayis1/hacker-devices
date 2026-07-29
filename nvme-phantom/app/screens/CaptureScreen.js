/**
 * screens/CaptureScreen.js — Start/stop capture, live command feed
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, FlatList } from 'react-native';

export default function CaptureScreen({ ble }) {
  const [capturing, setCapturing] = useState(false);
  const [session, setSession] = useState(0);
  const [commands, setCommands] = useState([]);
  const [summary, setSummary] = useState(null);
  const scrollRef = useRef(null);

  const handleStart = async () => {
    const sid = Math.floor(Math.random() * 0xFFFF);
    try {
      await ble.startCapture(sid);
      setSession(sid);
      setCapturing(true);
      setCommands([]);
    } catch (e) { alert('Error: ' + e.message); }
  };

  const handleStop = async () => {
    try {
      const r = await ble.stopCapture();
      setSummary(r);
      setCapturing(false);
    } catch (e) { alert('Error: ' + e.message); }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Capture</Text>
      <Text style={styles.session}>Session: 0x{session.toString(16).toUpperCase()}</Text>

      <View style={styles.buttonRow}>
        <TouchableOpacity style={[styles.button, capturing ? styles.buttonOff : styles.buttonOn]}
          onPress={capturing ? handleStop : handleStart}>
          <Text style={styles.buttonText}>{capturing ? '■ Stop' : '▶ Start'}</Text>
        </TouchableOpacity>
      </View>

      {summary && (
        <View style={styles.summaryCard}>
          <Text style={styles.summaryTitle}>Capture Summary</Text>
          <Text style={styles.summaryText}>Records: {summary.records}</Text>
          <Text style={styles.summaryText}>Duration: {summary.durationS}s</Text>
        </View>
      )}

      <Text style={styles.feedTitle}>Live Command Feed</Text>
      <View style={styles.feedContainer}>
        {commands.length === 0 ? (
          <Text style={styles.empty}>No commands captured yet.</Text>
        ) : (
          commands.map((cmd, i) => (
            <View key={i} style={styles.cmdRow}>
              <Text style={styles.cmdTime}>{cmd.time}</Text>
              <Text style={styles.cmdText}>{cmd.text}</Text>
            </View>
          ))
        )}
      </View>

      <Text style={styles.footer}>Capture format: PCAP-NG (DLT_NVME 0xFE) — jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 5 },
  session: { color: '#888', fontSize: 13, marginBottom: 15 },
  buttonRow: { flexDirection: 'row', marginBottom: 15 },
  button: { padding: 15, borderRadius: 8, width: 120, alignItems: 'center' },
  buttonOn: { backgroundColor: '#00AA00' },
  buttonOff: { backgroundColor: '#cc3300' },
  buttonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  summaryCard: { backgroundColor: '#16213e', borderRadius: 8, padding: 15, marginBottom: 15 },
  summaryTitle: { color: '#00AA00', fontSize: 16, fontWeight: 'bold', marginBottom: 5 },
  summaryText: { color: '#ccc', fontSize: 14, marginBottom: 3 },
  feedTitle: { fontSize: 16, color: '#888', marginBottom: 8 },
  feedContainer: { backgroundColor: '#0d1117', borderRadius: 6, padding: 10, minHeight: 200 },
  empty: { color: '#555', textAlign: 'center', marginTop: 50 },
  cmdRow: { flexDirection: 'row', paddingVertical: 3, borderBottomWidth: 0.5, borderBottomColor: '#222' },
  cmdTime: { color: '#666', fontSize: 11, width: 80 },
  cmdText: { color: '#aaa', fontSize: 12, flex: 1, fontFamily: 'monospace' },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 15 },
});