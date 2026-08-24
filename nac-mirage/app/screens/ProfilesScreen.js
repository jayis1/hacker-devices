// NAC Mirage profiles screen
// Author: jayis1

import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';

export default function ProfilesScreen({ profiles, selected, onSelect }) {
  return (
    <View>
      <Text style={styles.title}>Engagement Profiles</Text>
      <Text style={styles.subtitle}>Choose a prebuilt policy pack authored by jayis1.</Text>
      {profiles.map((profile) => {
        const active = selected === profile.id;
        return (
          <Pressable key={profile.id} style={[styles.card, active && styles.cardActive]} onPress={() => onSelect(profile.id)}>
            <View style={styles.header}>
              <Text style={styles.profileTitle}>{profile.title}</Text>
              <Text style={styles.risk}>{profile.risk}</Text>
            </View>
            <Text style={styles.summary}>{profile.summary}</Text>
            <Text style={styles.meta}>{active ? 'Selected for next deploy' : 'Tap to arm in lab mode'}</Text>
          </Pressable>
        );
      })}
    </View>
  );
}

const styles = StyleSheet.create({
  title: {
    color: '#f9fafb',
    fontSize: 24,
    fontWeight: '800',
    marginBottom: 8,
  },
  subtitle: {
    color: '#9ca3af',
    marginBottom: 16,
  },
  card: {
    backgroundColor: '#111827',
    borderRadius: 14,
    padding: 14,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#1f2937',
  },
  cardActive: {
    borderColor: '#3ddc97',
    backgroundColor: '#0b1b1b',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  profileTitle: {
    color: '#f9fafb',
    fontSize: 17,
    fontWeight: '700',
  },
  risk: {
    color: '#f59e0b',
    fontWeight: '700',
  },
  summary: {
    color: '#d1d5db',
    lineHeight: 20,
  },
  meta: {
    color: '#3ddc97',
    marginTop: 10,
    fontSize: 12,
  },
});
