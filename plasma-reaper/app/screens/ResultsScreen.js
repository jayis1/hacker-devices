/**
 * ResultsScreen.js — Sweep results dashboard
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: MIT
 *
 * Displays completed sweep results, statistics, and per-cell details.
 */

import React, { useState, useContext, useEffect } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet,
  ScrollView, FlatList,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { DeviceContext } from '../utils/deviceContext';
import { OUTCOME_LABELS, OUTCOME_COLORS } from '../utils/protocol';

export default function ResultsScreen() {
  const { connected, status } = useContext(DeviceContext);
  const [history, setHistory] = useState([]);

  useEffect(() => {
    if (status && !status.sweepRunning && status.totalShots > 0) {
      setHistory(prev => [{
        id: Date.now(),
        totalShots: status.totalShots,
        successShots: status.successShots,
        failureShots: status.failureShots,
        timestamp: new Date().toLocaleString(),
      }, ...prev].slice(0, 20));
    }
  }, [status?.sweepRunning]);

  const successRate = status && status.totalShots > 0
    ? ((status.successShots / status.totalShots) * 100).toFixed(1)
    : '0.0';

  return (
    <ScrollView style={styles.container}>
      <View style={styles.statsCard}>
        <Text style={styles.cardTitle}>Session Statistics</Text>
        <View style={styles.statsRow}>
          <StatBox label="Total Shots" value={status?.totalShots || 0} color="#2196F3" />
          <StatBox label="Successes" value={status?.successShots || 0} color="#4CAF50" />
          <StatBox label="Failures" value={status?.failureShots || 0} color="#F44336" />
        </View>
        <View style={styles.rateContainer}>
          <Text style={styles.rateLabel}>Success Rate:</Text>
          <Text style={styles.rateValue}>{successRate}%</Text>
        </View>
        <View style={styles.progressBar}>
          <View style={[styles.progressBarFill, {
            width: `${successRate}%`,
            backgroundColor: parseFloat(successRate) > 5 ? '#4CAF50' : '#FF9800'
          }]} />
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Recent Sweeps</Text>
        {history.length === 0 ? (
          <Text style={styles.emptyText}>No completed sweeps yet.</Text>
        ) : (
          history.map(item => (
            <View key={item.id} style={styles.historyItem}>
              <View style={styles.historyHeader}>
                <Text style={styles.historyTime}>{item.timestamp}</Text>
                <Text style={styles.historyRate}>
                  {((item.successShots / item.totalShots) * 100).toFixed(1)}%
                </Text>
              </View>
              <View style={styles.historyStats}>
                <Text style={styles.historyStat}>Shots: {item.totalShots}</Text>
                <Text style={[styles.historyStat, { color: '#4CAF50' }]}>OK: {item.successShots}</Text>
                <Text style={[styles.historyStat, { color: '#F44336' }]}>Fail: {item.failureShots}</Text>
              </View>
            </View>
          ))
        )}
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Outcome Legend</Text>
        {Object.entries(OUTCOME_LABELS).map(([code, label]) => (
          <View key={code} style={styles.legendRow}>
            <View style={[styles.legendColor, { backgroundColor: OUTCOME_COLORS[code] }]} />
            <Text style={styles.legendText}>{label}</Text>
          </View>
        ))}
      </View>

      <Text style={styles.footer}>Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

function StatBox({ label, value, color }) {
  return (
    <View style={styles.statBox}>
      <Text style={[styles.statValue, { color }]}>{value}</Text>
      <Text style={styles.statLabel}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  statsCard: { backgroundColor: '#fff', margin: 8, borderRadius: 8, padding: 16 },
  cardTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  statsRow: { flexDirection: 'row', justifyContent: 'space-around' },
  statBox: { alignItems: 'center', flex: 1 },
  statValue: { fontSize: 32, fontWeight: 'bold' },
  statLabel: { fontSize: 12, color: '#888', marginTop: 4 },
  rateContainer: { flexDirection: 'row', alignItems: 'center', marginTop: 16, marginBottom: 4 },
  rateLabel: { fontSize: 16, color: '#555' },
  rateValue: { fontSize: 20, fontWeight: 'bold', color: '#e91e63', marginLeft: 8 },
  progressBar: { height: 10, backgroundColor: '#e0e0e0', borderRadius: 5, marginTop: 4 },
  progressBarFill: { height: 10, borderRadius: 5 },
  section: { backgroundColor: '#fff', margin: 8, borderRadius: 8, padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  emptyText: { fontSize: 14, color: '#999', textAlign: 'center', padding: 20 },
  historyItem: { borderBottomWidth: 1, borderBottomColor: '#eee', paddingVertical: 12 },
  historyHeader: { flexDirection: 'row', justifyContent: 'space-between' },
  historyTime: { fontSize: 12, color: '#888' },
  historyRate: { fontSize: 16, fontWeight: 'bold', color: '#e91e63' },
  historyStats: { flexDirection: 'row', marginTop: 4 },
  historyStat: { fontSize: 12, color: '#666', marginRight: 16 },
  legendRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  legendColor: { width: 16, height: 16, borderRadius: 4, marginRight: 8 },
  legendText: { fontSize: 14 },
  footer: { textAlign: 'center', color: '#999', fontSize: 12, padding: 16 },
});