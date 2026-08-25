// MoCA Phantom node map screen
// Author: jayis1

import React from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { summarizeNodeRisk } from '../utils/protocol';

export default function NodeMapScreen({ nodes }) {
  return (
    <ScrollView style={styles.container}>
      <Text style={styles.heading}>Node Map</Text>
      {nodes.map((node) => (
        <View key={node.id} style={styles.card}>
          <View style={styles.row}>
            <Text style={styles.title}>Node {node.id}</Text>
            <Text style={[styles.badge, { color: node.privacyEnabled ? '#65d6a6' : '#ff647c' }]}>
              {node.privacyEnabled ? 'PRIVACY ON' : 'PRIVACY OFF'}
            </Text>
          </View>
          <Text style={styles.label}>{node.label}</Text>
          <Text style={styles.meta}>Role: {node.role} · RSSI: {node.rssi} dBm</Text>
          <Text style={styles.risk}>Risk: {summarizeNodeRisk(node)}</Text>
        </View>
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1 },
  heading: {
    color: '#edf4ff',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  card: {
    backgroundColor: '#0f1d31',
    borderRadius: 14,
    padding: 14,
    marginBottom: 12,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  title: {
    color: '#edf4ff',
    fontWeight: '700',
    fontSize: 16,
  },
  badge: {
    fontWeight: '700',
    fontSize: 12,
  },
  label: {
    color: '#c9d7e8',
    marginTop: 8,
  },
  meta: {
    color: '#8aa4c8',
    marginTop: 6,
  },
  risk: {
    color: '#ffc857',
    marginTop: 8,
    fontWeight: '600',
  },
});
