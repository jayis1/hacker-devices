// MoCA Phantom capture screen
// Author: jayis1

import React from 'react';
import { ScrollView, StyleSheet, Text } from 'react-native';
import EventList from '../components/EventList';

export default function CaptureScreen({ events }) {
  return (
    <ScrollView style={styles.container}>
      <Text style={styles.heading}>Capture Feed</Text>
      <Text style={styles.helper}>Recent device-side findings and operator-relevant alerts.</Text>
      <EventList events={events} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1 },
  heading: {
    color: '#edf4ff',
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 8,
  },
  helper: {
    color: '#8aa4c8',
    marginBottom: 14,
  },
});
