// SafetyScreen.js - PoE Whisper safety checklist
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SafetyScreen({ checklist }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Authorized-Use Safety Checklist</Text>
      {checklist.map((item) => (
        <View key={item} style={styles.card}>
          <Text style={styles.bullet}>•</Text>
          <Text style={styles.text}>{item}</Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 10 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  card: {
    flexDirection: 'row',
    gap: 10,
    padding: 14,
    backgroundColor: '#111827',
    borderRadius: 14,
    borderWidth: 1,
    borderColor: '#1f2937',
  },
  bullet: { color: '#22d3ee', fontWeight: '800', fontSize: 18 },
  text: { color: '#cbd5e1', lineHeight: 20, flex: 1 },
});
