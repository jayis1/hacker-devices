// Captures screen for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import CaptureList from '../components/CaptureList';

export default function CapturesScreen({ captures }) {
  const modifiedCount = captures.filter((item) => item.modified).length;

  return (
    <View>
      <Text style={styles.sectionTitle}>Recent Process-Data Captures</Text>
      <Text style={styles.copy}>Modified frames: {modifiedCount} · Observed frames: {captures.length - modifiedCount}</Text>
      <CaptureList captures={captures} />
    </View>
  );
}

const styles = StyleSheet.create({
  sectionTitle: {
    color: '#eaf3fe',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 12,
  },
  copy: {
    color: '#94a9c3',
    marginBottom: 14,
    lineHeight: 21,
  },
});
