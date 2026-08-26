// DP AUX Phantom profiles screen
// Author: jayis1

import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';

export default function ProfilesScreen({ profiles, selected, onSelect }) {
  return (
    <View style={styles.wrapper}>
      {profiles.map((profile) => {
        const active = profile.id === selected;
        return (
          <Pressable
            key={profile.id}
            onPress={() => onSelect(profile.id)}
            style={[styles.card, active && styles.cardActive]}
          >
            <Text style={[styles.title, active && styles.titleActive]}>{profile.title}</Text>
            <Text style={styles.summary}>{profile.summary}</Text>
          </Pressable>
        );
      })}
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    gap: 14,
  },
  card: {
    backgroundColor: '#111827',
    borderRadius: 18,
    padding: 16,
    borderWidth: 1,
    borderColor: '#1f2937',
    gap: 8,
  },
  cardActive: {
    borderColor: '#22d3ee',
    backgroundColor: '#082f49',
  },
  title: {
    color: '#f8fafc',
    fontSize: 18,
    fontWeight: '800',
  },
  titleActive: {
    color: '#ecfeff',
  },
  summary: {
    color: '#9ca3af',
    lineHeight: 20,
  },
});
