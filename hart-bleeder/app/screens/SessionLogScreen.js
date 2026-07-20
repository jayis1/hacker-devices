/**
 * hart-bleeder — SessionLogScreen.js
 * Session log viewer: displays encrypted log lines received via the
 * BLE C2 link, with timestamps and severity color-coding.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  SafeAreaView, StatusBar, Share,
} from 'react-native';

export default function SessionLogScreen({ bleManager, sessionLog }) {
  const [filter, setFilter] = useState('ALL');

  const filters = ['ALL', 'CAP', 'FAULT', 'BTN', 'INFO'];

  const formatLine = (line, i) => {
    const ts = new Date().toLocaleTimeString();
    let severity = 'INFO';
    let color = '#4a90d9';
    if (line.includes('FAULT')) { severity = 'FAULT'; color = '#e94560'; }
    else if (line.includes('CAP')) { severity = 'CAP'; color = '#10b981'; }
    else if (line.includes('BTN')) { severity = 'BTN'; color = '#f59e0b'; }
    return { id: i.toString(), text: line, ts, severity, color };
  };

  const allLines = (sessionLog || []).map(formatLine);
  const filtered = filter === 'ALL'
    ? allLines
    : allLines.filter((l) => l.severity === filter);

  const handleShare = async () => {
    const text = (sessionLog || []).join('\n');
    try {
      await Share.share({ message: text, title: 'HART-Bleeder Session Log' });
    } catch (e) { /* user cancelled */ }
  };

  const renderItem = ({ item }) => (
    <View style={[styles.logRow, { borderLeftColor: item.color }]}>
      <Text style={styles.logTs}>[{item.ts}]</Text>
      <Text style={[styles.logText, { color: item.color }]}>{item.text}</Text>
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <View style={styles.toolbar}>
        <Text style={styles.title}>Session Log</Text>
        <TouchableOpacity style={styles.shareBtn} onPress={handleShare}>
          <Text style={styles.shareBtnText}>Share</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.filterRow}>
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
      </View>

      <FlatList
        data={filtered}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
        ListEmptyComponent={
          <Text style={styles.empty}>No log entries yet. Connect and start capturing.</Text>
        }
        contentContainerStyle={{ paddingBottom: 20 }}
      />

      <Text style={styles.disclaimer}>
        ⚠ Logs may contain sensitive process data. Handle with appropriate
        care and purge after authorized engagement concludes.
      </Text>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  toolbar: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 10 },
  title: { color: '#e94560', fontSize: 18, fontWeight: 'bold' },
  shareBtn: { backgroundColor: '#2d2d44', padding: 8, borderRadius: 8 },
  shareBtnText: { color: '#fff', fontSize: 12 },
  filterRow: { flexDirection: 'row', marginBottom: 10 },
  filterBtn: { backgroundColor: '#1a1a2e', padding: 8, borderRadius: 6, marginRight: 6, borderWidth: 1, borderColor: '#2d2d44' },
  filterActive: { backgroundColor: '#e94560', borderColor: '#e94560' },
  filterText: { color: '#888', fontSize: 11 },
  filterTextActive: { color: '#fff', fontWeight: 'bold' },
  logRow: { flexDirection: 'row', backgroundColor: '#1a1a2e', borderRadius: 6, padding: 10, marginBottom: 4, borderLeftWidth: 3 },
  logTs: { color: '#555', fontSize: 9, marginRight: 8, fontFamily: 'monospace' },
  logText: { fontSize: 11, fontFamily: 'monospace', flex: 1 },
  empty: { color: '#555', textAlign: 'center', marginTop: 40, fontSize: 13 },
  disclaimer: { color: '#f59e0b', fontSize: 9, textAlign: 'center', marginTop: 8 },
});