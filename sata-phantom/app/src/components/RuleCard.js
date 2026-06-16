/**
 * RuleCard.js — Rule Display Card Component
 * Author: jayis1
 */

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const ACTION_INFO = {
  0: { label: 'MONITOR', color: '#48f' },
  1: { label: 'DROP', color: '#f44' },
  2: { label: 'CORRUPT', color: '#fa4' },
  3: { label: 'INJECT', color: '#f0f' },
};

const RuleCard = ({ rule, index, onDelete }) => {
  const action = ACTION_INFO[rule.action] || ACTION_INFO[0];
  const dirLabel = rule.direction === 0 ? 'READ' : rule.direction === 1 ? 'WRITE' : 'BOTH';
  const lbaStart = typeof rule.lbaStart === 'number' ? `0x${rule.lbaStart.toString(16)}` : rule.lbaStart;

  return (
    <View style={[styles.container, !rule.enabled && styles.disabled]}>
      <View style={styles.header}>
        <Text style={styles.name}>{rule.name || `Rule #${index + 1}`}</Text>
        <View style={[styles.actionBadge, { borderColor: action.color, backgroundColor: action.color + '22' }]}>
          <Text style={[styles.actionText, { color: action.color }]}>{action.label}</Text>
        </View>
      </View>
      <View style={styles.details}>
        <Text style={styles.detailText}>LBA: {lbaStart}</Text>
        <Text style={styles.detailText}>OP: 0x{typeof rule.opcode === 'number' ? rule.opcode.toString(16).padStart(2, '0') : rule.opcode}</Text>
        <Text style={styles.detailText}>{dirLabel}</Text>
        <Text style={styles.detailText}>Prio: {rule.priority}</Text>
      </View>
      <TouchableOpacity style={styles.deleteBtn} onPress={onDelete}>
        <Icon name="delete" size={16} color="#f44" />
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#12122a', borderRadius: 10, padding: 12,
    marginBottom: 8, borderWidth: 1, borderColor: '#2a2a4a',
    position: 'relative',
  },
  disabled: { opacity: 0.5 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  name: { color: '#e0e0e0', fontSize: 14, fontWeight: 'bold', flex: 1 },
  actionBadge: { borderRadius: 6, paddingHorizontal: 8, paddingVertical: 3, borderWidth: 1 },
  actionText: { fontSize: 11, fontWeight: 'bold' },
  details: { flexDirection: 'row', gap: 12, marginTop: 8, flexWrap: 'wrap' },
  detailText: { color: '#888', fontSize: 11, fontFamily: 'monospace' },
  deleteBtn: { position: 'absolute', top: 8, right: 8 },
});

export default RuleCard;
