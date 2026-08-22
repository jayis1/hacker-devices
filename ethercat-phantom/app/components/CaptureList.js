// CaptureList component for EtherCAT Phantom
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function CaptureList({ captures }) {
  return (
    <View>
      {captures.map((capture) => (
        <View key={`${capture.cycle}-${capture.object}`} style={styles.row}>
          <Text style={styles.title}>Cycle {capture.cycle} · Slave {capture.slave}</Text>
          <Text style={styles.meta}>{capture.object} · {capture.before} → {capture.after}</Text>
          <Text style={[styles.badge, capture.modified ? styles.modified : styles.passive]}>
            {capture.modified ? 'Modified' : 'Observed'}
          </Text>
        </View>
      ))}
      <Text style={styles.author}>Author: jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  row: {
    backgroundColor: '#101725',
    borderRadius: 12,
    padding: 12,
    marginBottom: 10,
  },
  title: {
    color: '#e6effa',
    fontWeight: '700',
    marginBottom: 4,
  },
  meta: {
    color: '#8ba0bc',
    marginBottom: 8,
  },
  badge: {
    alignSelf: 'flex-start',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 999,
    fontSize: 12,
    overflow: 'hidden',
  },
  modified: {
    backgroundColor: '#4b1d1d',
    color: '#ffbeb8',
  },
  passive: {
    backgroundColor: '#123124',
    color: '#bff0cf',
  },
  author: {
    color: '#445267',
    marginTop: 8,
    fontSize: 11,
  },
});
