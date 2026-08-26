// DP AUX Phantom event list
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function EventList({ events }) {
  return (
    <View style={styles.wrapper}>
      {events.map((event) => (
        <View key={`${event.time}-${event.title}`} style={styles.row}>
          <View style={[styles.badge, event.severity === 'alert' ? styles.alert : styles.info]}>
            <Text style={styles.badgeText}>{event.severity.toUpperCase()}</Text>
          </View>
          <View style={styles.content}>
            <Text style={styles.title}>{event.title}</Text>
            <Text style={styles.meta}>{event.time} · {event.detail}</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    gap: 12,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    gap: 12,
    backgroundColor: '#0f172a',
    borderWidth: 1,
    borderColor: '#1e293b',
    borderRadius: 16,
    padding: 14,
  },
  badge: {
    borderRadius: 999,
    paddingVertical: 6,
    paddingHorizontal: 10,
  },
  alert: {
    backgroundColor: '#3f111a',
  },
  info: {
    backgroundColor: '#083344',
  },
  badgeText: {
    color: '#e2e8f0',
    fontWeight: '800',
    fontSize: 11,
  },
  content: {
    flex: 1,
    gap: 6,
  },
  title: {
    color: '#f8fafc',
    fontWeight: '800',
  },
  meta: {
    color: '#94a3b8',
    lineHeight: 19,
  },
});
