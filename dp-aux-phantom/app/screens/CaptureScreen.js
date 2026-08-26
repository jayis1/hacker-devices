// DP AUX Phantom capture screen
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import EventList from '../components/EventList';

export default function CaptureScreen({ events }) {
  return (
    <View style={styles.wrapper}>
      <Text style={styles.title}>AUX / HPD event stream</Text>
      <Text style={styles.subtitle}>
        Correlate EDID reads, link-training writes, and HPD transitions to expose brittle host or dock logic.
      </Text>
      <EventList events={events} />
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    gap: 14,
  },
  title: {
    color: '#f8fafc',
    fontSize: 22,
    fontWeight: '800',
  },
  subtitle: {
    color: '#94a3b8',
    lineHeight: 20,
  },
});
