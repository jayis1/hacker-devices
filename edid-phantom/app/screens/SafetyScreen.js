// SafetyScreen.js - EDID Phantom safety checklist screen
// Author: jayis1
// Copyright (c) 2026 jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SafetyScreen({ toggles }) {
  const checklist = [
    'Confirm you are working on owned or explicitly authorized display infrastructure.',
    'Keep HPD pulse rate below target tolerance and verify rollback timers before live use.',
    'Do not fuzz HDCP-protected links on production systems without change approval.',
    toggles.cecGuard
      ? 'CEC guard rails are enabled, reducing accidental menu navigation on nearby displays.'
      : 'CEC guard rails are disabled; lab-only mode should be treated as high risk.',
  ];

  return (
    <View style={styles.card}>
      <Text style={styles.heading}>Safety & Ethics</Text>
      {checklist.map((line) => (
        <Text key={line} style={styles.item}>• {line}</Text>
      ))}
      <View style={styles.banner}>
        <Text style={styles.bannerText}>Authorized use only. Created by jayis1 for defensive research, validation, and red-team exercises.</Text>
      </View>
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
  item: {
    color: '#cbd5e1',
    lineHeight: 21,
  },
  banner: {
    backgroundColor: '#3f1d0d',
    borderRadius: 14,
    padding: 14,
  },
  bannerText: {
    color: '#fed7aa',
    lineHeight: 20,
    fontWeight: '700',
  },
});
