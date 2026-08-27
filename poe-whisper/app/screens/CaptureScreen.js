// CaptureScreen.js - PoE Whisper capture view
// Author: jayis1
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import EventList from '../components/EventList';

export default function CaptureScreen({ events }) {
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Telemetry and Event Capture</Text>
      <Text style={styles.subtitle}>Current signatures, LLDP activity, and bounded power actions.</Text>
      <EventList events={events} />
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { gap: 12 },
  title: { color: '#f8fafc', fontSize: 22, fontWeight: '900' },
  subtitle: { color: '#94a3b8', lineHeight: 20 },
});
