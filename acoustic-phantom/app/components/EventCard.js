/**
 * ACOUSTIC-PHANTOM — Event Card Component
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Displays a single classified acoustic event with timestamp,
 * label, confidence bar, and top-5 alternatives.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { getLabel } from '../utils/protocol';

const EVENT_COLORS = {
  keyboard: '#4CAF50',
  hdd:      '#2196F3',
  printer:  '#FF9800',
  smps:     '#9C27B0',
  relay:    '#F44336',
  unknown:  '#666',
};

function getEventColor(label) {
  if (label.length === 1 || ['SPACE','ENTER','SHIFT','TAB','ESC','CTRL','ALT',
      'BKSP','CAPS'].includes(label)) return EVENT_COLORS.keyboard;
  if (label.startsWith('SEEK') || label.startsWith('SPIN') ||
      label.startsWith('IDLE') || label.startsWith('PARK') ||
      label.startsWith('READ') || label.startsWith('WRITE')) return EVENT_COLORS.hdd;
  if (label.startsWith('DOT') || label.startsWith('CHAR') ||
      label.startsWith('LINE') || label.startsWith('CR') ||
      label.startsWith('FF') || label.startsWith('BOLD') ||
      label.startsWith('COMPRESSED')) return EVENT_COLORS.printer;
  if (label.startsWith('CPU') || label.startsWith('CRYPTO') ||
      label.startsWith('MEMORY') || label.startsWith('DMA')) return EVENT_COLORS.smps;
  if (label.startsWith('RELAY')) return EVENT_COLORS.relay;
  return EVENT_COLORS.unknown;
}

function formatTimestamp(ms) {
  const seconds = Math.floor(ms / 1000);
  const millis = ms % 1000;
  const mins = Math.floor(seconds / 60);
  const secs = seconds % 60;
  return `${String(mins).padStart(2,'0')}:${String(secs).padStart(2,'0')}.${String(millis).padStart(3,'0')}`;
}

export default function EventCard({ event, index }) {
  const color = getEventColor(event.label);
  const confPct = Math.round(event.confidence * 100);

  return (
    <View style={[styles.card, { borderLeftColor: color }]}>
      <View style={styles.header}>
        <Text style={styles.timestamp}>{formatTimestamp(event.timestamp)}</Text>
        <Text style={styles.index}>#{index + 1}</Text>
      </View>

      <View style={styles.body}>
        <Text style={[styles.label, { color }]}>{event.label}</Text>
        <View style={styles.confidenceBar}>
          <View style={[styles.confidenceFill, { width: `${confPct}%`, backgroundColor: color }]} />
        </View>
        <Text style={styles.confidenceText}>{confPct}% confidence</Text>
      </View>

      {event.interEvent > 0 && (
        <Text style={styles.timing}>Δ {event.interEvent}ms since last event</Text>
      )}

      {/* Top-5 alternatives */}
      {event.top5Conf && event.top5Conf[1] > 0.05 && (
        <View style={styles.alternatives}>
          <Text style={styles.altLabel}>Alternatives:</Text>
          {event.top5Ids.slice(1, 5).map((id, i) => (
            <Text key={i} style={styles.altItem}>
              {getLabel(id)} ({Math.round(event.top5Conf[i + 1] * 100)}%)
            </Text>
          ))}
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#222',
    borderRadius: 8,
    padding: 12,
    marginBottom: 6,
    borderLeftWidth: 4,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  timestamp: { color: '#888', fontSize: 12, fontFamily: 'monospace' },
  index: { color: '#555', fontSize: 12 },
  body: { flexDirection: 'row', alignItems: 'center', gap: 12 },
  label: {
    fontSize: 20,
    fontWeight: 'bold',
    fontFamily: 'monospace',
    minWidth: 60,
  },
  confidenceBar: {
    flex: 1,
    height: 8,
    backgroundColor: '#333',
    borderRadius: 4,
    overflow: 'hidden',
  },
  confidenceFill: { height: '100%', borderRadius: 4 },
  confidenceText: { color: '#999', fontSize: 11, minWidth: 80 },
  timing: { color: '#666', fontSize: 11, marginTop: 6 },
  alternatives: { marginTop: 8, paddingTop: 8, borderTopWidth: 1, borderTopColor: '#333' },
  altLabel: { color: '#555', fontSize: 10, marginBottom: 4 },
  altItem: { color: '#777', fontSize: 11 },
});