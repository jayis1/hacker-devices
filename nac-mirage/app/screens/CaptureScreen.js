// NAC Mirage capture screen
// Author: jayis1

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import EventList from '../components/EventList';

export default function CaptureScreen({ events }) {
  return (
    <View>
      <Text style={styles.title}>Capture Timeline</Text>
      <Text style={styles.subtitle}>Packet decisions, LLDP rewrites, and PoE events from NAC Mirage.</Text>
      <EventList events={events} />
    </View>
  );
}

const styles = StyleSheet.create({
  title: {
    color: '#f9fafb',
    fontSize: 24,
    fontWeight: '800',
    marginBottom: 8,
  },
  subtitle: {
    color: '#9ca3af',
    marginBottom: 16,
  },
});
