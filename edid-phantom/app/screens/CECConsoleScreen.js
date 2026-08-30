// CECConsoleScreen.js - EDID Phantom CEC operator panel
// Author: jayis1
// Copyright (c) 2026 jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function CECConsoleScreen({ actions }) {
  return (
    <View style={styles.card}>
      <Text style={styles.heading}>CEC Console</Text>
      {actions.map((action) => (
        <View key={action.title} style={styles.actionCard}>
          <Text style={styles.actionTitle}>{action.title}</Text>
          <Text style={styles.actionDetail}>{action.detail}</Text>
        </View>
      ))}
      <Text style={styles.footer}>All transmitted control actions are intended for authorized lab validation only.</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    gap: 12,
    backgroundColor: '#0f172a',
    borderColor: '#1e293b',
    borderWidth: 1,
    borderRadius: 18,
    padding: 16,
  },
  heading: {
    color: '#f8fafc',
    fontSize: 18,
    fontWeight: '800',
  },
  actionCard: {
    backgroundColor: '#111827',
    borderColor: '#1f2937',
    borderWidth: 1,
    borderRadius: 14,
    padding: 14,
    gap: 6,
  },
  actionTitle: {
    color: '#e2e8f0',
    fontWeight: '800',
  },
  actionDetail: {
    color: '#94a3b8',
    lineHeight: 20,
  },
  footer: {
    color: '#fbbf24',
    lineHeight: 20,
  },
});
