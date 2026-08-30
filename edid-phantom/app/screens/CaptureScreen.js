// CaptureScreen.js - EDID Phantom capture feed screen
// Author: jayis1
// Copyright (c) 2026 jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function CaptureScreen({ events }) {
  return (
    <View style={styles.card}>
      <Text style={styles.heading}>Live Bus Activity</Text>
      {events.map((event) => (
        <View key={`${event.time}-${event.bus}`} style={styles.row}>
          <Text style={styles.time}>{event.time}</Text>
          <View style={styles.body}>
            <Text style={styles.bus}>{event.bus}</Text>
            <Text style={styles.detail}>{event.detail}</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    gap: 12,
    backgroundColor: '#0f172a',
    borderColor: '#1e293b',
    borderWidth: 1,
    borderRadius: 18,
    padding: 16,
  },
  heading: {
    color: '#f8fafc',
    fontSize: 18,
    fontWeight: '800',
  },
  row: {
    flexDirection: 'row',
    gap: 12,
    alignItems: 'flex-start',
    borderBottomWidth: 1,
    borderBottomColor: '#1e293b',
    paddingBottom: 10,
  },
  time: {
    color: '#67e8f9',
    fontWeight: '800',
    width: 68,
  },
  body: {
    flex: 1,
    gap: 4,
  },
  bus: {
    color: '#f8fafc',
    fontWeight: '800',
  },
  detail: {
    color: '#94a3b8',
    lineHeight: 20,
  },
});
