// Dockruptor Identity Screen
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

export default function IdentityScreen({ session }) {
  const { identities } = session;

  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Identity and eMarker View</Text>
      <View style={styles.card}>
        <Text style={styles.label}>Host</Text>
        <Text style={styles.value}>{identities.host}</Text>
        <Text style={styles.label}>Dock</Text>
        <Text style={styles.value}>{identities.dock}</Text>
        <Text style={styles.label}>Original Cable</Text>
        <Text style={styles.value}>{identities.cableOriginal}</Text>
        <Text style={styles.label}>Spoofed Cable</Text>
        <Text style={styles.valueAccent}>{identities.cableSpoofed}</Text>
        <Text style={styles.label}>Advertised SVIDs</Text>
        <Text style={styles.value}>{identities.svids.join(', ')}</Text>
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
    gap: 6,
  },
  label: {
    color: '#7fd1ff',
    fontWeight: '800',
    marginTop: 10,
  },
  value: {
    color: '#d9e7fb',
    lineHeight: 22,
  },
  valueAccent: {
    color: '#ffd166',
    lineHeight: 22,
  },
});
