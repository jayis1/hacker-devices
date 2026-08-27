// ToggleCard.js - PoE Whisper control card
// Author: jayis1
import React from 'react';
import { Pressable, Text, StyleSheet } from 'react-native';

export default function ToggleCard({ title, description, active, onPress }) {
  return (
    <Pressable style={[styles.card, active && styles.active]} onPress={onPress}>
      <Text style={styles.title}>{title}</Text>
      <Text style={styles.desc}>{description}</Text>
      <Text style={styles.state}>{active ? 'Enabled' : 'Disabled'}</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#0f172a',
    borderColor: '#1e293b',
    borderWidth: 1,
    borderRadius: 14,
    padding: 14,
    gap: 8,
  },
  active: {
    borderColor: '#f59e0b',
    backgroundColor: '#3f2a03',
  },
  title: { color: '#f8fafc', fontWeight: '800', fontSize: 16 },
  desc: { color: '#94a3b8', lineHeight: 18 },
  state: { color: '#fde68a', fontWeight: '700' },
});
