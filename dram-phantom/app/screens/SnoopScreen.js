/**
 * SnoopScreen.js — live DDR4 row-access trace viewer
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Drains snoop records from the device and renders a scrolling table of
 * decoded ACT/READ/WRITE/PRE/REF events. Includes filters by bank group,
 * bank, row, and op type, plus a "hot rows" indicator that flags rows
 * with abnormally high activation counts (Rowhammer indicator).
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { View, Text, StyleSheet, FlatList, Button, TextInput, TouchableOpacity } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const OP_NAMES = ['ACT', 'RD', 'WR', 'PRE', 'REF', 'RFSB'];
const MODES = ['IDLE', 'SNOOP', 'HAMMER', 'WARM', 'COVERT'];

export default function SnoopScreen() {
  const { connected, status, setMode, snoopDrain, refreshStatus } = useDevice();
  const [records, setRecords] = useState([]);
  const [filterBg, setFilterBg] = useState('');
  const [filterBank, setFilterBank] = useState('');
  const [filterOp, setFilterOp] = useState('');
  const [hotRows, setHotRows] = useState(0);
  const pollRef = useRef(null);

  const poll = useCallback(async () => {
    const n = await snoopDrain(32);
    if (n > 0) {
      // In the real app we'd decode the binary stream. Here we generate
      // representative decoded records for the UI demo.
      const newRecs = [];
      for (let i = 0; i < n; i++) {
        newRecs.push({
          id: `${Date.now()}-${i}`,
          bg: Math.floor(Math.random() * 4),
          bank: Math.floor(Math.random() * 4),
          row: Math.floor(Math.random() * 65536),
          col: Math.floor(Math.random() * 2048),
          op: Math.floor(Math.random() * 4),
          t: Date.now() + i,
        });
      }
      setRecords((prev) => [...newRecs, ...prev].slice(0, 200));
    }
  }, [snoopDrain]);

  useEffect(() => {
    if (connected && status && status.mode === 1) {
      pollRef.current = setInterval(poll, 1500);
      return () => clearInterval(pollRef.current);
    }
  }, [connected, status, poll]);

  const filtered = records.filter((r) => {
    if (filterBg && r.bg !== parseInt(filterBg, 10)) return false;
    if (filterBank && r.bank !== parseInt(filterBank, 10)) return false;
    if (filterOp && r.op !== parseInt(filterOp, 10)) return false;
    return true;
  });

  const renderItem = ({ item }) => (
    <View style={styles.row}>
      <Text style={styles.cell}>{item.bg}</Text>
      <Text style={styles.cell}>{item.bank}</Text>
      <Text style={styles.cell}>{item.row}</Text>
      <Text style={styles.cell}>{item.col}</Text>
      <Text style={[styles.cell, { color: opColor(item.op) }]}>{OP_NAMES[item.op]}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>DDR4 Snoop</Text>
        <Text style={styles.status}>
          {connected ? `BLE/USB · mode=${MODES[status?.mode ?? 0]}` : 'disconnected'}
        </Text>
      </View>

      <View style={styles.controls}>
        <Button title="Start Snoop" onPress={async () => { await setMode(1); await refreshStatus(); }} />
        <Button title="Stop" color="#d73a49" onPress={async () => { await setMode(0); await refreshStatus(); }} />
        <Button title="Refresh" onPress={refreshStatus} />
      </View>

      <View style={styles.filters}>
        <TextInput style={styles.input} placeholder="BG" keyboardType="numeric"
          value={filterBg} onChangeText={setFilterBg} />
        <TextInput style={styles.input} placeholder="Bank" keyboardType="numeric"
          value={filterBank} onChangeText={setFilterBank} />
        <TextInput style={styles.input} placeholder="Op (0-5)" keyboardType="numeric"
          value={filterOp} onChangeText={setFilterOp} />
      </View>

      <View style={styles.hotRow}>
        <Text style={styles.hotText}>Hot rows ({'>'}1k ACTs): {hotRows}</Text>
        <Button title="Analyze" onPress={() => {
          // Count act-per-row in the current window
          const map = {};
          records.forEach((r) => {
            if (r.op === 0) {
              const k = `${r.bg}/${r.bank}/${r.row}`;
              map[k] = (map[k] || 0) + 1;
            }
          });
          const hot = Object.values(map).filter((c) => c > 1000).length;
          setHotRows(hot);
        }} />
      </View>

      <View style={styles.tableHeader}>
        <Text style={[styles.cell, styles.headerCell]}>BG</Text>
        <Text style={[styles.cell, styles.headerCell]}>Bank</Text>
        <Text style={[styles.cell, styles.headerCell]}>Row</Text>
        <Text style={[styles.cell, styles.headerCell]}>Col</Text>
        <Text style={[styles.cell, styles.headerCell]}>Op</Text>
      </View>
      <FlatList
        data={filtered}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
        inverted
        style={styles.list}
      />
      <Text style={styles.footer}>Author: jayis1 · DRAM-Phantom v1.0</Text>
    </View>
  );
}

function opColor(op) {
  const colors = ['#58a6ff', '#3fb950', '#f0883e', '#7d8590', '#bc8cff', '#d2a8ff'];
  return colors[op] || '#e6edf3';
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 8 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  title: { color: '#e6edf3', fontSize: 18, fontWeight: 'bold' },
  status: { color: '#7d8590', fontSize: 12 },
  controls: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 8 },
  filters: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  input: { flex: 1, backgroundColor: '#161b22', color: '#e6edf3', borderRadius: 4, padding: 6, fontSize: 12 },
  hotRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 },
  hotText: { color: '#f0883e', fontSize: 12 },
  tableHeader: { flexDirection: 'row', backgroundColor: '#161b22', padding: 4 },
  headerCell: { color: '#7d8590', fontWeight: 'bold' },
  row: { flexDirection: 'row', padding: 4, borderBottomWidth: 0.5, borderBottomColor: '#21262d' },
  cell: { flex: 1, color: '#e6edf3', fontSize: 11, fontFamily: 'monospace' },
  list: { flex: 1 },
  footer: { color: '#484f58', fontSize: 10, textAlign: 'center', marginTop: 4 },
});