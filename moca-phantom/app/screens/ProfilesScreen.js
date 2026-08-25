// MoCA Phantom profiles screen
// Author: jayis1

import React from 'react';
import { StyleSheet, Text, TouchableOpacity, View } from 'react-native';

const profiles = [
  { key: 'survey', title: 'Survey', description: 'Passive discovery and privacy-state mapping.' },
  { key: 'riserAudit', title: 'Riser Audit', description: 'Inline observation with leakage tracing.' },
  { key: 'activeValidation', title: 'Active Validation', description: 'Authorized bounded manipulation testing.' },
];

export default function ProfilesScreen({ selectedProfile, onSelectProfile, safeMode }) {
  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Profiles</Text>
      <Text style={styles.helper}>Safe mode is currently {safeMode ? 'enabled' : 'disabled'}.</Text>
      {profiles.map((profile) => (
        <TouchableOpacity
          key={profile.key}
          style={[styles.card, selectedProfile === profile.key && styles.cardActive]}
          onPress={() => onSelectProfile(profile.key)}
        >
          <Text style={styles.title}>{profile.title}</Text>
          <Text style={styles.description}>{profile.description}</Text>
        </TouchableOpacity>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1 },
  heading: {
    color: '#edf4ff',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 8,
  },
  helper: {
    color: '#8aa4c8',
    marginBottom: 14,
  },
  card: {
    backgroundColor: '#0f1d31',
    borderRadius: 14,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#1f3557',
  },
  cardActive: {
    borderColor: '#28c0f0',
  },
  title: {
    color: '#edf4ff',
    fontWeight: '700',
    marginBottom: 6,
  },
  description: {
    color: '#a2b6d3',
    lineHeight: 20,
  },
});
