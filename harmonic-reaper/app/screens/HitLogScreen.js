/**
 * HitLogScreen.js — scrollable list of detected hits with details
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React from 'react';
import { View, Text, StyleSheet, FlatList } from 'react-native';
import { CLASSIFY } from '../utils/protocol';

const CLASSIFY_LABELS = ['—', 'SEMICONDUCTOR', 'METAL', 'AMBIGUOUS'];
const CLASSIFY_COLORS = ['#666', '#ff3333', '#888888', '#ffaa00'];

function formatTime(ms) {
  const sec = Math.floor(ms / 1000);
  const m = Math.floor(sec / 60);
  const s = sec % 60;
  return `${m}:${s.toString().padStart(2, '0')}.${(ms % 1000).toString().padStart(3, '0')}`;
}

function HitItem({ hit, index }) {
  const color = CLASSIFY_COLORS[hit.classify] || '#666';
  const label = CLASSIFY_LABELS[hit.classify] || '—';

  return (
    <View style={styles.hitCard}>
      <View style={styles.hitRow}>
        <Text style={styles.hitNumber}>#{index + 1}</Text>
        <Text style={styles.hitTime}>{formatTime(hit.timestamp_ms)}</Text>
        <Text style={[styles.hitVerdict, { color }]}>{label}</Text>
      </View>
      <View style={styles.hitDetails}>
        <Text style={styles.detail}>2f: {hit.p2_dbfs} dBFS</Text>
        <Text style={styles.detail}>3f: {hit.p3_dbfs} dBFS</Text>
        <Text style={styles.detail}>Ratio: {hit.ratio_db > 0 ? '+' : ''}{hit.ratio_db} dB</Text>
      </View>
      <View style={styles.hitDetails}>
        <Text style={styles.detail}>Heading: {hit.yaw_deg}°</Text>
        <Text style={styles.detail}>Pitch: {hit.pitch_deg}°</Text>
        <Text style={styles.detail}>TX: {hit.tx_power_dbm} dBm</Text>
      </View>
    </View>
  );
}

export default function HitLogScreen({ hits }) {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Hit Log</Text>
      <Text style={styles.subtitle}>
        {hits.length} hit{hits.length !== 1 ? 's' : ''} recorded this session
      </Text>

      {hits.length === 0 ? (
        <View style={styles.empty}>
          <Text style={styles.emptyText}>No hits yet. Start sweeping to detect.</Text>
        </View>
      ) : (
        <FlatList
          data={hits}
          renderItem={({ item, index }) => <HitItem hit={item} index={index} />}
          keyExtractor={(_, i) => i.toString()}
          contentContainerStyle={{ paddingBottom: 20 }}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
    padding: 16,
  },
  title: {
    color: '#00ff88',
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  subtitle: {
    color: '#555',
    fontSize: 12,
    marginBottom: 12,
  },
  empty: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  emptyText: {
    color: '#444',
    fontSize: 14,
  },
  hitCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  hitRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  hitNumber: {
    color: '#00ff88',
    fontSize: 16,
    fontWeight: 'bold',
  },
  hitTime: {
    color: '#888',
    fontSize: 12,
  },
  hitVerdict: {
    fontSize: 12,
    fontWeight: 'bold',
  },
  hitDetails: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  detail: {
    color: '#aaa',
    fontSize: 12,
  },
});