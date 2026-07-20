/**
 * hart-bleeder — LiveCaptureScreen.js
 * Real-time capture feed of HART frames sniffed off the loop.
 * Displays decoded command, address, byte count, direction (CMD/RSP),
 * and payload hex dump for each frame as it arrives.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  SafeAreaView, StatusBar, Alert,
} from 'react-native';

// HART command name lookup (subset, matching firmware hart_cmd_name)
const CMD_NAMES = {
  0: 'Read Unique ID', 1: 'Read PV', 2: 'Read Loop Current',
  3: 'Read DV0', 6: 'Poll Sub-Device', 9: 'Read PV Status',
  11: 'Read Unique ID (assoc)', 12: 'Read Tag/Desc/Date',
  13: 'Read Tag/Desc/Date', 14: 'Read PV Sensor Info',
  17: 'Write Tag', 35: 'Write PV', 48: 'Read Additional Info',
};

function decodeFrame(bytes) {
  // Find start byte (0x06 or 0x82) after preambles
  let i = 0;
  while (i < bytes.length && bytes[i] === 0xFF) i++;
  if (i >= bytes.length) return null;
  const start = bytes[i++];
  const isLong = start === 0x82;
  if (start !== 0x06 && start !== 0x82) return null;
  const addr = bytes[i++] || 0;
  let cmdByte = bytes[i++] || 0;
  const isResponse = (cmdByte & 0x80) ? true : false;
  const cmd = cmdByte & 0x7F;
  const bytecnt = bytes[i++] || 0;
  let payloadStart = i;
  if (isLong) { i += 2; payloadStart = i; }  // skip expansion status
  const dataLen = isLong ? Math.max(0, bytecnt - 2) : bytecnt;
  const payload = bytes.slice(payloadStart, payloadStart + dataLen);
  return { isLong, addr, cmd, isResponse, bytecnt, payload, raw: bytes };
}

function hexDump(bytes, max) {
  const n = Math.min(bytes.length, max || 32);
  let s = '';
  for (let i = 0; i < n; i++) s += bytes[i].toString(16).padStart(2, '0') + ' ';
  return s.trim();
}

export default function LiveCaptureScreen({ bleManager, connected, capturedFrames }) {
  const [capturing, setCapturing] = useState(false);
  const [frames, setFrames] = useState([]);
  const flatRef = useRef(null);

  useEffect(() => {
    if (capturedFrames && capturedFrames.length > 0) {
      const latest = capturedFrames[capturedFrames.length - 1];
      const decoded = decodeFrame(latest);
      if (decoded) {
        setFrames((prev) => [...prev.slice(-199), decoded]);
      }
    }
  }, [capturedFrames]);

  const handleStart = async (ms) => {
    if (!connected) { Alert.alert('Not connected'); return; }
    setCapturing(true);
    try { await bleManager.startCapture(ms); }
    catch (e) { Alert.alert('Error', e.message); }
    setTimeout(() => setCapturing(false), ms + 500);
  };

  const renderItem = ({ item, index }) => (
    <View style={styles.frameCard}>
      <View style={styles.frameHeader}>
        <Text style={styles.frameIndex}>#{index + 1}</Text>
        <Text style={[styles.frameDir, { color: item.isResponse ? '#10b981' : '#4a90d9' }]}>
          {item.isResponse ? 'RSP' : 'CMD'}
        </Text>
        <Text style={styles.frameCmd}>Cmd {item.cmd}</Text>
        <Text style={styles.frameName}>{CMD_NAMES[item.cmd] || 'Unknown'}</Text>
      </View>
      <View style={styles.frameBody}>
        <Text style={styles.frameMeta}>
          Addr {item.addr}  {item.isLong ? 'LONG' : 'SHORT'}  Len {item.bytecnt}
        </Text>
        <Text style={styles.frameHex}>{hexDump(item.payload, 48)}</Text>
        <Text style={styles.frameRaw}>RAW: {hexDump(item.raw, 24)}</Text>
      </View>
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <View style={styles.toolbar}>
        <Text style={styles.title}>Live Capture</Text>
        {capturing ? (
          <Text style={styles.recording}>● REC</Text>
        ) : null}
      </View>

      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.capBtn} onPress={() => handleStart(10000)}>
          <Text style={styles.capBtnText}>Capture 10s</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.capBtn} onPress={() => handleStart(30000)}>
          <Text style={styles.capBtnText}>Capture 30s</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.capBtn, styles.clearBtn]} onPress={() => setFrames([])}>
          <Text style={styles.capBtnText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        ref={flatRef}
        data={frames}
        keyExtractor={(item, i) => i.toString()}
        renderItem={renderItem}
        ListEmptyComponent={<Text style={styles.empty}>No frames captured yet.</Text>}
        onContentSizeChange={() => flatRef.current?.scrollToEnd()}
        contentContainerStyle={{ paddingBottom: 20 }}
      />

      <Text style={styles.disclaimer}>
        ⚠ Captured frames may contain process control commands. Authorized
        monitoring only — do not capture on loops you do not own or operate.
      </Text>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  toolbar: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 10 },
  title: { color: '#e94560', fontSize: 18, fontWeight: 'bold' },
  recording: { color: '#e94560', fontWeight: 'bold', fontSize: 16 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  capBtn: { backgroundColor: '#2d2d44', padding: 10, borderRadius: 8, flex: 1, marginHorizontal: 3, alignItems: 'center' },
  clearBtn: { backgroundColor: '#444' },
  capBtnText: { color: '#fff', fontWeight: '600', fontSize: 12 },
  frameCard: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 8, borderWidth: 1, borderColor: '#2d2d44' },
  frameHeader: { flexDirection: 'row', alignItems: 'center', marginBottom: 6 },
  frameIndex: { color: '#666', fontSize: 10, marginRight: 8 },
  frameDir: { fontWeight: 'bold', fontSize: 11, marginRight: 8 },
  frameCmd: { color: '#f59e0b', fontSize: 12, marginRight: 8 },
  frameName: { color: '#aaa', fontSize: 11 },
  frameBody: { marginLeft: 4 },
  frameMeta: { color: '#888', fontSize: 10, marginBottom: 4 },
  frameHex: { color: '#4a90d9', fontSize: 10, fontFamily: 'monospace', marginBottom: 2 },
  frameRaw: { color: '#555', fontSize: 9, fontFamily: 'monospace' },
  empty: { color: '#555', textAlign: 'center', marginTop: 40, fontSize: 13 },
  disclaimer: { color: '#f59e0b', fontSize: 9, textAlign: 'center', marginTop: 8 },
});