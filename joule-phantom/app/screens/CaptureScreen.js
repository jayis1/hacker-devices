/**
 * CaptureScreen.js — Live SMBus frame capture viewer for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Displays the stream of captured SMBus/PMBus frames in real time,
 * colour-coded by direction and flags (modified / injected / NACK).
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
} from 'react-native';

export default function CaptureScreen({ frames }) {
  const [filter, setFilter] = useState('ALL');
  const [paused, setPaused] = useState(false);

  const filters = ['ALL', 'HOST', 'BATT', 'MODIFIED', 'INJECTED'];

  const filtered = frames.filter((f) => {
    if (filter === 'ALL') return true;
    if (filter === 'MODIFIED') return f.modified;
    if (filter === 'INJECTED') return f.injected;
    return f.port === filter;
  });

  const displayed = paused ? filtered : filtered.slice(-100).reverse();

  const renderItem = ({ item }) => (
    <View style={[styles.row, item.modified && styles.rowModified,
                  item.injected && styles.rowInjected,
                  item.nack && styles.rowNack]}>
      <Text style={styles.ts}>{(item.ts / 1000).toFixed(1)}s</Text>
      <Text style={[styles.port, { color: item.port === 'HOST' ? '#08f' : '#0f0' }]}>
        {item.port}
      </Text>
      <Text style={styles.dir}>{item.dir}</Text>
      <Text style={styles.cmd}>{item.cmdName}</Text>
      <Text style={styles.data}>
        {item.rbuf.length > 0
          ? 'R:' + item.rbuf.map((b) => b.toString(16).padStart(2, '0')).join(' ')
          : item.wbuf.length > 0
            ? 'W:' + item.wbuf.map((b) => b.toString(16).padStart(2, '0')).join(' ')
            : '-'}
      </Text>
      {item.modified && <Text style={styles.flag}>⚡</Text>}
      {item.injected && <Text style={styles.flag}>💉</Text>}
      {item.nack && <Text style={styles.flag}>✗</Text>}
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        {filters.map((f) => (
          <TouchableOpacity
            key={f}
            style={[styles.filterBtn, filter === f && styles.filterActive]}
            onPress={() => setFilter(f)}
          >
            <Text style={[styles.filterText, filter === f && styles.filterTextActive]}>
              {f}
            </Text>
          </TouchableOpacity>
        ))}
        <TouchableOpacity style={styles.pauseBtn} onPress={() => setPaused(!paused)}>
          <Text style={styles.pauseText}>{paused ? '▶' : '⏸'}</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.header}>
        <Text style={styles.headerText}>Time</Text>
        <Text style={styles.headerText}>Port</Text>
        <Text style={styles.headerText}>Dir</Text>
        <Text style={styles.headerText}>Command</Text>
        <Text style={styles.headerText}>Data</Text>
      </View>

      {displayed.length === 0 ? (
        <Text style={styles.empty}>No frames captured yet. Connect device and insert between host and battery.</Text>
      ) : (
        <FlatList
          data={displayed}
          keyExtractor={(_, i) => String(i)}
          renderItem={renderItem}
        />
      )}

      <Text style={styles.footer}>jayis1 — GPL-2.0 — {frames.length} total frames</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  toolbar: { flexDirection: 'row', padding: 8, backgroundColor: '#16213e' },
  filterBtn: {
    borderWidth: 1, borderColor: '#333', borderRadius: 6,
    paddingHorizontal: 10, paddingVertical: 4, marginRight: 6,
  },
  filterActive: { borderColor: '#e94560' },
  filterText: { color: '#888', fontSize: 11 },
  filterTextActive: { color: '#e94560', fontWeight: 'bold' },
  pauseBtn: { marginLeft: 'auto', paddingHorizontal: 12 },
  pauseText: { color: '#e94560', fontSize: 18 },
  header: {
    flexDirection: 'row', backgroundColor: '#1a1a2e', padding: 8,
    borderBottomWidth: 1, borderBottomColor: '#333',
  },
  headerText: { color: '#666', fontSize: 10, flex: 1 },
  row: {
    flexDirection: 'row', alignItems: 'center',
    padding: 8, borderBottomWidth: 1, borderBottomColor: '#1a1a2e',
  },
  rowModified: { backgroundColor: '#e9456011' },
  rowInjected: { backgroundColor: '#08f1' },
  rowNack: { backgroundColor: '#f0011' },
  ts: { color: '#666', fontSize: 10, width: 50 },
  port: { fontSize: 10, width: 40, fontWeight: 'bold' },
  dir: { color: '#888', fontSize: 10, width: 50 },
  cmd: { color: '#fff', fontSize: 11, width: 100 },
  data: { color: '#0f0', fontSize: 10, flex: 1, fontFamily: 'monospace' },
  flag: { fontSize: 12, marginLeft: 4 },
  empty: { color: '#555', fontSize: 13, padding: 20, fontStyle: 'italic', textAlign: 'center' },
  footer: { color: '#333', fontSize: 9, textAlign: 'center', padding: 6 },
});