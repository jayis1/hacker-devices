// RuleEditor component for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function RuleEditor({ rule }) {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>{rule.label}</Text>
      <Text style={styles.line}>Object: {rule.object}</Text>
      <Text style={styles.line}>Action: {rule.action}</Text>
      <Text style={styles.line}>Enabled: {rule.enabled ? 'Yes' : 'No'}</Text>
      <Text style={styles.author}>Author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#0d1420',
    borderColor: '#1f2c40',
    borderWidth: 1,
    borderRadius: 12,
    padding: 12,
    marginBottom: 10,
  },
  title: {
    color: '#e3f2ff',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 6,
  },
  line: {
    color: '#8ea7c7',
    fontSize: 13,
    marginBottom: 2,
  },
  author: {
    color: '#445267',
    marginTop: 6,
    fontSize: 11,
  },
});
