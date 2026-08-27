// EventList.js - PoE Whisper event renderer
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function EventList({ events }) {
  return (
    <View style={styles.wrap}>
      {events.map((event) => (
        <View key={`${event.ts}-${event.message}`} style={styles.item}>
          <Text style={styles.ts}>{event.ts}</Text>
          <View style={styles.body}>
            <Text style={styles.code}>{event.code}</Text>
            <Text style={styles.msg}>{event.message}</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 10 },
  item: {
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 12,
    borderWidth: 1,
    borderColor: '#1f2937',
    flexDirection: 'row',
    gap: 12,
  },
  ts: { color: '#22d3ee', fontWeight: '700', width: 58 },
  body: { flex: 1, gap: 3 },
  code: { color: '#e2e8f0', fontWeight: '700' },
  msg: { color: '#94a3b8', lineHeight: 18 },
});
