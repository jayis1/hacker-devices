// Dockruptor Safety Screen
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';

export default function SafetyScreen({ session, onAcknowledgeSafety }) {
  const { safety } = session;

  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Safety Controls</Text>
      <View style={styles.card}>
        <Text style={styles.row}>Relay bypass: {safety.relayBypass ? 'ASSERTED' : 'inactive'}</Text>
        <Text style={styles.row}>Safe mode: {safety.safeMode ? 'enabled' : 'disabled'}</Text>
        <Text style={styles.row}>Acknowledged: {safety.acknowledged ? 'yes' : 'no'}</Text>
        <Text style={styles.notice}>{safety.advisory}</Text>
        <Pressable style={styles.button} onPress={onAcknowledgeSafety}>
          <Text style={styles.buttonText}>Acknowledge Authorized-Use Advisory</Text>
        </Pressable>
      </View>
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
  card: {
    backgroundColor: '#102338',
    borderRadius: 16,
    padding: 16,
    gap: 12,
  },
  row: {
    color: '#d9e7fb',
    fontWeight: '700',
  },
  notice: {
    color: '#ffd166',
    lineHeight: 22,
  },
  button: {
    marginTop: 6,
    backgroundColor: '#1d5cff',
    borderRadius: 12,
    paddingVertical: 12,
    paddingHorizontal: 14,
  },
  buttonText: {
    color: '#ffffff',
    fontWeight: '800',
    textAlign: 'center',
  },
});
