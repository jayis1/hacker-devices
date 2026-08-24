// NAC Mirage safety screen
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SafetyScreen({ checklist }) {
  return (
    <View>
      <Text style={styles.title}>Safety and Authorization</Text>
      <Text style={styles.subtitle}>NAC Mirage is for authorized use only. Review before arming hardware.</Text>
      {checklist.map((item, index) => (
        <View key={`${index}-${item}`} style={styles.item}>
          <Text style={styles.bullet}>•</Text>
          <Text style={styles.text}>{item}</Text>
        </View>
      ))}
      <Text style={styles.footer}>Author and creator: jayis1</Text>
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
  item: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    marginBottom: 12,
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 12,
  },
  bullet: {
    color: '#ef4444',
    fontSize: 18,
    marginRight: 10,
  },
  text: {
    flex: 1,
    color: '#e5e7eb',
    lineHeight: 20,
  },
  footer: {
    color: '#3ddc97',
    marginTop: 8,
  },
});
