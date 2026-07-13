/**
 * eddy-phantom — SessionLogScreen.js
 * Timestamped history of all captured keystrokes with export
 * and filtering capabilities.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useMemo } from 'react';
import {
  View, Text, StyleSheet, FlatList, SafeAreaView,
  StatusBar, TouchableOpacity, Share, Alert,
} from 'react-native';

export default function SessionLogScreen({ sessionLog }) {
  const [filterMin, setFilterMin] = useState(0);
  const [sortBy, setSortBy] = useState('time');

  // Filter and sort keystrokes
  const filteredKeys = useMemo(() => {
    let result = sessionLog.filter(k => k.confidence >= filterMin);
    if (sortBy === 'confidence') {
      result = [...result].sort((a, b) => b.confidence - a.confidence);
    }
    return result;
  }, [sessionLog, filterMin, sortBy]);

  // Export session as text
  const exportSession = async () => {
    const text = sessionLog.map(k =>
      `[${k.time}] ${k.char} (conf: ${k.confidence}%)`
    ).join('\n');

    try {
      await Share.share({
        message: `Eddy-Phantom Session Log\n${new Date().toISOString()}\n\n${text}`,
        title: 'Eddy-Phantom Session Export',
      });
    } catch (error) {
      Alert.alert('Export error', error.message);
    }
  };

  // Export as JSON
  const exportJSON = async () => {
    const json = JSON.stringify(sessionLog, null, 2);
    try {
      await Share.share({
        message: json,
        title: 'Eddy-Phantom JSON Export',
      });
    } catch (error) {
      Alert.alert('Export error', error.message);
    }
  };

  // Clear session
  const clearSession = () => {
    Alert.alert(
      'Clear Session',
      'Remove all captured keystrokes from this session?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Clear', onPress: () => { /* would clear state */ } },
      ]
    );
  };

  const renderItem = ({ item }) => (
    <View style={styles.logRow}>
      <Text style={styles.logTime}>{item.time}</Text>
      <Text style={styles.logKey}>
        {item.char === '\n' ? '⏎' : item.char === ' ' ? '␣' : item.char}
      </Text>
      <Text style={styles.logScan}>0x{item.scancode.toString(16).toUpperCase().padStart(2, '0')}</Text>
      <View style={styles.confBar}>
        <View style={[styles.confFill, {
          width: `${item.confidence}%`,
          backgroundColor: item.confidence >= 90 ? '#4ade80' :
                           item.confidence >= 70 ? '#f0a500' : '#ef4444',
        }]} />
      </View>
      <Text style={[
        styles.logConf,
        { color: item.confidence >= 90 ? '#4ade80' :
                 item.confidence >= 70 ? '#f0a500' : '#ef4444' }
      ]}>
        {item.confidence}%
      </Text>
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      {/* Summary */}
      <View style={styles.summaryCard}>
        <Text style={styles.summaryTitle}>Session Summary</Text>
        <View style={styles.summaryGrid}>
          <StatBox label="Total Keys" value={sessionLog.length} />
          <StatBox label="High Conf (>90%)"
            value={sessionLog.filter(k => k.confidence >= 90).length} />
          <StatBox label="Med Conf (70-89%)"
            value={sessionLog.filter(k => k.confidence >= 70 && k.confidence < 90).length} />
          <StatBox label="Low Conf (<70%)"
            value={sessionLog.filter(k => k.confidence < 70).length} />
        </View>
      </View>

      {/* Filter controls */}
      <View style={styles.filterBar}>
        <Text style={styles.filterLabel}>Min Confidence:</Text>
        {[0, 50, 70, 90].map(val => (
          <TouchableOpacity
            key={val}
            style={[styles.filterBtn, filterMin === val && styles.filterBtnActive]}
            onPress={() => setFilterMin(val)}
          >
            <Text style={[styles.filterBtnText,
              filterMin === val && styles.filterBtnTextActive]}>
              {val}%
            </Text>
          </TouchableOpacity>
        ))}
        <TouchableOpacity
          style={styles.sortBtn}
          onPress={() => setSortBy(sortBy === 'time' ? 'confidence' : 'time')}
        >
          <Text style={styles.sortText}>
            Sort: {sortBy === 'time' ? 'Time' : 'Confidence'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Export buttons */}
      <View style={styles.exportBar}>
        <TouchableOpacity style={styles.exportBtn} onPress={exportSession}>
          <Text style={styles.exportText}>📄 Export Text</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.exportBtn} onPress={exportJSON}>
          <Text style={styles.exportText}>{'{}'} Export JSON</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.exportBtn, { borderColor: '#e94560' }]}
          onPress={clearSession}>
          <Text style={[styles.exportText, { color: '#e94560' }]}>🗑 Clear</Text>
        </TouchableOpacity>
      </View>

      {/* Session log list */}
      <FlatList
        data={filteredKeys}
        renderItem={renderItem}
        keyExtractor={(item) => item.id.toString()}
        style={styles.logList}
        ListEmptyComponent={
          <Text style={styles.emptyText}>No keystrokes captured yet</Text>
        }
      />
    </SafeAreaView>
  );
}

function StatBox({ label, value }) {
  return (
    <View style={styles.statBox}>
      <Text style={styles.statValue}>{value}</Text>
      <Text style={styles.statLabel}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  summaryCard: {
    backgroundColor: '#1a1a2e', margin: 12, borderRadius: 12,
    padding: 16, borderWidth: 1, borderColor: '#2a2a3e',
  },
  summaryTitle: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  summaryGrid: { flexDirection: 'row', justifyContent: 'space-between' },
  statBox: { alignItems: 'center', flex: 1 },
  statValue: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  statLabel: { color: '#888', fontSize: 10, marginTop: 4, textAlign: 'center' },
  filterBar: {
    flexDirection: 'row', alignItems: 'center', paddingHorizontal: 12,
    gap: 6, marginBottom: 8, flexWrap: 'wrap',
  },
  filterLabel: { color: '#888', fontSize: 12, marginRight: 4 },
  filterBtn: {
    padding: 6, borderRadius: 4, borderWidth: 1, borderColor: '#2a2a3e',
  },
  filterBtnActive: { borderColor: '#4a90d9', backgroundColor: '#4a90d922' },
  filterBtnText: { color: '#666', fontSize: 11 },
  filterBtnTextActive: { color: '#4a90d9' },
  sortBtn: { padding: 6, borderRadius: 4, borderWidth: 1, borderColor: '#2a2a3e', marginLeft: 'auto' },
  sortText: { color: '#888', fontSize: 11 },
  exportBar: { flexDirection: 'row', paddingHorizontal: 12, gap: 8, marginBottom: 8 },
  exportBtn: {
    padding: 8, borderRadius: 6, borderWidth: 1, borderColor: '#2a2a3e',
    backgroundColor: '#1a1a2e',
  },
  exportText: { color: '#ccc', fontSize: 12 },
  logList: { flex: 1, paddingHorizontal: 12 },
  logRow: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 6, padding: 10,
    marginBottom: 3, borderWidth: 1, borderColor: '#2a2a3e',
  },
  logTime: { color: '#555', fontSize: 10, width: 75 },
  logKey: { color: '#fff', fontSize: 16, fontWeight: 'bold', width: 40, textAlign: 'center' },
  logScan: { color: '#666', fontSize: 10, width: 50, fontFamily: 'monospace' },
  confBar: {
    flex: 1, height: 4, backgroundColor: '#2a2a3e', borderRadius: 2,
    marginHorizontal: 8, overflow: 'hidden',
  },
  confFill: { height: '100%' },
  logConf: { fontSize: 10, width: 30, textAlign: 'right' },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 40, fontSize: 14 },
});