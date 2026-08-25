// MoCA Phantom event list component
// Author: jayis1

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

const severityColor = {
  high: '#ff647c',
  medium: '#ffc857',
  low: '#65d6a6',
};

export default function EventList({ events }) {
  return (
    <View style={styles.container}>
      {events.map((event) => (
        <View key={`${event.ts}-${event.title}`} style={styles.row}>
          <View style={[styles.dot, { backgroundColor: severityColor[event.severity] || '#28c0f0' }]} />
          <View style={styles.body}>
            <Text style={styles.title}>{event.title}</Text>
            <Text style={styles.meta}>{event.ts} · {event.severity.toUpperCase()}</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    gap: 10,
  },
  row: {
    flexDirection: 'row',
    gap: 12,
    padding: 12,
    backgroundColor: '#0f1d31',
    borderRadius: 14,
  },
  dot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginTop: 6,
  },
  body: {
    flex: 1,
  },
  title: {
    color: '#edf4ff',
    fontWeight: '600',
  },
  meta: {
    color: '#8aa4c8',
    marginTop: 4,
    fontSize: 12,
  },
});
