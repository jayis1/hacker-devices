// OverviewScreen.js - EDID Phantom app overview screen
// Author: jayis1
// Copyright (c) 2026 jayis1
import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';

export default function OverviewScreen({ profiles, selectedProfile, onSelectProfile, report }) {
  return (
    <View style={styles.card}>
      <Text style={styles.heading}>Scenario Profiles</Text>
      {profiles.map((profile) => (
        <Pressable
          key={profile.key}
          style={[styles.profileCard, selectedProfile === profile.key && styles.selected]}
          onPress={() => onSelectProfile(profile.key)}
        >
          <Text style={styles.profileTitle}>{profile.title}</Text>
          <Text style={styles.profileSummary}>{profile.summary}</Text>
          <Text style={styles.profileRisk}>Risk posture: {profile.risk}</Text>
        </Pressable>
      ))}
      <View style={styles.reportBox}>
        <Text style={styles.heading}>Mutation Build Preview</Text>
        <Text style={styles.meta}>Severity: {report.severity}</Text>
        <Text style={styles.meta}>Seed: {report.seed}</Text>
        <Text style={styles.meta}>Enabled paths: {report.enabled || 'none'}</Text>
        {report.preview.map((line) => (
          <Text key={line} style={styles.bullet}>• {line}</Text>
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
  profileCard: {
    backgroundColor: '#111827',
    borderRadius: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
    padding: 14,
    gap: 6,
  },
  selected: {
    borderColor: '#22d3ee',
  },
  profileTitle: {
    color: '#e2e8f0',
    fontSize: 16,
    fontWeight: '800',
  },
  profileSummary: {
    color: '#94a3b8',
    lineHeight: 20,
  },
  profileRisk: {
    color: '#fbbf24',
    fontWeight: '700',
  },
  reportBox: {
    marginTop: 8,
    backgroundColor: '#020617',
    borderRadius: 14,
    padding: 14,
    gap: 6,
  },
  meta: {
    color: '#67e8f9',
  },
  bullet: {
    color: '#cbd5e1',
    lineHeight: 20,
  },
});
