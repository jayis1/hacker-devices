// NAC Mirage EventList component
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function EventList({ events }) {
  return (
    <View style={styles.wrapper}>
      {events.map((event) => (
        <View key={event.id} style={styles.row}>
          <View style={styles.timePill}>
            <Text style={styles.timeText}>{event.ts}</Text>
          </View>
          <View style={styles.body}>
            <Text style={styles.tag}>{event.tag}</Text>
            <Text style={styles.text}>{event.text}</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    backgroundColor: '#111827',
    borderRadius: 14,
    padding: 12,
    gap: 10,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'flex-start',
  },
  timePill: {
    backgroundColor: '#0f766e',
    borderRadius: 10,
    paddingHorizontal: 10,
    paddingVertical: 6,
    marginRight: 10,
  },
  timeText: {
    color: '#ecfeff',
    fontSize: 12,
    fontWeight: '700',
  },
  body: {
    flex: 1,
  },
  tag: {
    color: '#f59e0b',
    fontWeight: '700',
    marginBottom: 3,
  },
  text: {
    color: '#e5e7eb',
    lineHeight: 20,
  },
});
