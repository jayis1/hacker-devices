// ProfilesScreen.js - PoE Whisper profile selector
// Author: jayis1
import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';

export default function ProfilesScreen({ profiles, selected, onSelect }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Research Profiles</Text>
      {profiles.map((profile) => (
        <Pressable
          key={profile.id}
          style={[styles.card, selected === profile.id && styles.cardActive]}
          onPress={() => onSelect(profile.id)}
        >
          <Text style={styles.name}>{profile.name}</Text>
          <Text style={styles.desc}>{profile.description}</Text>
          <Text style={styles.meta}>{profile.meta}</Text>
        </Pressable>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 12 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  card: {
    backgroundColor: '#111827',
    borderRadius: 14,
    padding: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
    gap: 6,
  },
  cardActive: {
    borderColor: '#22d3ee',
    backgroundColor: '#082f49',
  },
  name: { color: '#f8fafc', fontWeight: '800' },
  desc: { color: '#cbd5e1', lineHeight: 18 },
  meta: { color: '#94a3b8', fontSize: 12 },
});
