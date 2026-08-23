// Dockruptor Capture Screen
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';
import EventList from '../components/EventList';

export default function CaptureScreen({ session }) {
  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Capture Timeline</Text>
      <Text style={styles.subheading}>
        Structured evidence log for authorized engagements. Export over BLE or Wi-Fi in the full product build.
      </Text>
      <EventList events={session.events} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { gap: 16 },
  heading: {
    color: '#e5f1ff',
    fontSize: 22,
    fontWeight: '800',
  },
  subheading: {
    color: '#98aecb',
    lineHeight: 20,
  },
});
