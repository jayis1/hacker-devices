/**
 * screens/CaptureViewerScreen.js — Browse PCAP-NG capture sessions
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, FlatList, Picker } from 'react-native';
import RNFS from 'react-native-fs';

const OPCODE_NAMES = {
  0x00: 'Flush', 0x01: 'Write', 0x02: 'Read', 0x05: 'Compare',
  0x06: 'Identify', 0x08: 'Write Zeroes', 0x09: 'DSM',
  0x0E: 'FW Commit', 0x10: 'FW Download', 0x7C: 'Format',
  0x7D: 'SecSend', 0x7E: 'SecRecv', 0x81: 'SecSend', 0x82: 'SecRecv',
  0x84: 'Sanitize',
};

export default function CaptureViewerScreen({ ble }) {
  const [sessions, setSessions] = useState([]);
  const [selectedSession, setSelectedSession] = useState(null);
  const [records, setRecords] = useState([]);

  useEffect(() => {
    loadSessions();
  }, []);

  const loadSessions = async () => {
    // In a full implementation this would query the device for session
    // metadata via BLE_CMD_GET_CAPMETA.  Here we list files from the
    // app's document directory (captures synced from the device).
    try {
      const files = await RNFS.readDir(RNFS.DocumentDirectoryPath);
      const caps = files.filter(f => f.name.endsWith('.pcapng'));
      setSessions(caps.map(f => ({ name: f.name, path: f.path, size: f.size })));
    } catch (e) {
      setSessions([]);
    }
  };

  const renderRecord = ({ item, index }) => (
    <View style={styles.recordRow}>
      <Text style={styles.recIdx}>#{index + 1}</Text>
      <Text style={styles.recTime}>{item.timestamp}</Text>
      <Text style={styles.recOp}>{OPCODE_NAMES[item.opcode] || '0x' + item.opcode.toString(16)}</Text>
      <Text style={styles.recLba}>LBA:0x{item.lba?.toString(16) || '---'}</Text>
      <Text style={styles.recCid}>CID:{item.cid}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.header}>Capture Viewer</Text>

      <Text style={styles.label}>Session</Text>
      <Picker selectedValue={selectedSession} style={styles.picker}
        onValueChange={(v) => setSelectedSession(v)}>
        <Picker.Item label="-- Select a session --" value={null} />
        {sessions.map((s, i) => (
          <Picker.Item key={i} label={`${s.name} (${(s.size/1024).toFixed(1)} KB)`} value={s.path} />
        ))}
      </Picker>

      <View style={styles.statsBox}>
        <Text style={styles.statsText}>Records: {records.length}</Text>
        <Text style={styles.statsText}>Format: PCAP-NG (DLT_NVME)</Text>
      </View>

      <FlatList
        data={records}
        renderItem={renderRecord}
        keyExtractor={(item, i) => i.toString()}
        style={styles.list}
        ListEmptyComponent={<Text style={styles.empty}>No records. Start a capture first.</Text>}
      />

      <Text style={styles.footer}>PCAP-NG viewer — Wireshark-compatible — jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 10 },
  label: { color: '#aaa', fontSize: 14, marginBottom: 5 },
  picker: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, marginBottom: 10 },
  statsBox: { backgroundColor: '#16213e', borderRadius: 6, padding: 10, marginBottom: 10 },
  statsText: { color: '#888', fontSize: 12, marginBottom: 3 },
  list: { flex: 1, backgroundColor: '#0d1117', borderRadius: 6 },
  recordRow: { flexDirection: 'row', paddingVertical: 6, paddingHorizontal: 10, borderBottomWidth: 0.5, borderBottomColor: '#222' },
  recIdx: { color: '#666', fontSize: 11, width: 40 },
  recTime: { color: '#888', fontSize: 11, width: 80 },
  recOp: { color: '#00AA00', fontSize: 12, width: 80, fontFamily: 'monospace', fontWeight: 'bold' },
  recLba: { color: '#aaa', fontSize: 11, flex: 1, fontFamily: 'monospace' },
  recCid: { color: '#888', fontSize: 11, width: 60 },
  empty: { color: '#555', textAlign: 'center', marginTop: 50 },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 10 },
});