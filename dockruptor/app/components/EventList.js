// Dockruptor Event List
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

export default function EventList({ events }) {
  return (
    <View style={styles.container}>
      {events.map((event) => (
        <View key={event.id} style={styles.row}>
          <View style={styles.meta}>
            <Text style={styles.time}>{event.time}</Text>
            <Text style={styles.direction}>{event.direction}</Text>
          </View>
          <View style={styles.body}>
            <Text style={styles.type}>{event.type.toUpperCase()}</Text>
            <Text style={styles.summary}>{event.summary}</Text>
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
    backgroundColor: '#102338',
    borderRadius: 14,
    padding: 12,
  },
  meta: {
    width: 86,
  },
  time: {
    color: '#7fd1ff',
    fontWeight: '700',
    marginBottom: 4,
  },
  direction: {
    color: '#7d95b6',
    fontSize: 12,
  },
  body: {
    flex: 1,
    gap: 4,
  },
  type: {
    color: '#d8e8ff',
    fontSize: 12,
    fontWeight: '800',
    letterSpacing: 0.6,
  },
  summary: {
    color: '#aabdd8',
    lineHeight: 20,
  },
});
