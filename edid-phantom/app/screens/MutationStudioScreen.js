// MutationStudioScreen.js - EDID Phantom mutation controls
// Author: jayis1
// Copyright (c) 2026 jayis1
import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';

function ToggleRow({ label, value, onPress }) {
  return (
    <Pressable style={[styles.toggle, value && styles.toggleActive]} onPress={onPress}>
      <View>
        <Text style={styles.toggleTitle}>{label}</Text>
        <Text style={styles.toggleValue}>{value ? 'Enabled' : 'Disabled'}</Text>
      </View>
      <Text style={styles.toggleGlyph}>{value ? 'ON' : 'OFF'}</Text>
    </Pressable>
  );
}

export default function MutationStudioScreen({ toggles, onToggle, report }) {
  return (
    <View style={styles.card}>
      <Text style={styles.heading}>Mutation Studio</Text>
      <ToggleRow label="Preserve Vendor Block" value={toggles.preserveVendor} onPress={() => onToggle('preserveVendor')} />
      <ToggleRow label="Inject DDC Timing Jitter" value={toggles.injectLatency} onPress={() => onToggle('injectLatency')} />
      <ToggleRow label="Spoof Elevated HDR Capabilities" value={toggles.spoofHdr} onPress={() => onToggle('spoofHdr')} />
      <ToggleRow label="Guard CEC Transmission" value={toggles.cecGuard} onPress={() => onToggle('cecGuard')} />
      <View style={styles.previewBox}>
        <Text style={styles.previewTitle}>Compiler Preview</Text>
        <Text style={styles.previewLine}>Severity posture: {report.severity}</Text>
        <Text style={styles.previewLine}>Derived seed: {report.seed}</Text>
        {report.preview.map((line) => (
          <Text key={line} style={styles.previewBullet}>• {line}</Text>
        ))}
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
  toggle: {
    backgroundColor: '#111827',
    borderColor: '#1f2937',
    borderWidth: 1,
    borderRadius: 14,
    padding: 14,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  toggleActive: {
    borderColor: '#22d3ee',
  },
  toggleTitle: {
    color: '#e2e8f0',
    fontWeight: '800',
  },
  toggleValue: {
    color: '#94a3b8',
  },
  toggleGlyph: {
    color: '#67e8f9',
    fontWeight: '900',
  },
  previewBox: {
    backgroundColor: '#020617',
    borderRadius: 14,
    padding: 14,
    gap: 6,
  },
  previewTitle: {
    color: '#f8fafc',
    fontWeight: '800',
  },
  previewLine: {
    color: '#67e8f9',
  },
  previewBullet: {
    color: '#cbd5e1',
    lineHeight: 20,
  },
});
