// Dockruptor Policy Screen
// Author: jayis1
// SPDX-License-Identifier: MIT

import React from 'react';
import { StyleSheet, Switch, Text, TextInput, View } from 'react-native';

export default function PolicyScreen({ session, onMutatePolicy }) {
  const { policy } = session;

  return (
    <View style={styles.container}>
      <Text style={styles.heading}>Mutation Policy</Text>
      <View style={styles.card}>
        <View style={styles.row}>
          <Text style={styles.label}>Clamp source capabilities to travel-dock ceiling</Text>
          <Switch
            value={policy.powerClamp}
            onValueChange={(value) => onMutatePolicy({ ...policy, powerClamp: value })}
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Delay DisplayPort alt-mode release</Text>
          <TextInput
            style={styles.input}
            value={String(policy.altModeDelayTicks)}
            onChangeText={(value) =>
              onMutatePolicy({ ...policy, altModeDelayTicks: Number(value) || 0 })
            }
            keyboardType="number-pad"
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Charge-only fallback</Text>
          <Switch
            value={policy.chargeOnly}
            onValueChange={(value) => onMutatePolicy({ ...policy, chargeOnly: value })}
          />
        </View>
        <View style={styles.rowColumn}>
          <Text style={styles.label}>Spoofed cable identity</Text>
          <TextInput
            multiline
            style={[styles.input, styles.largeInput]}
            value={policy.cableSpoof}
            onChangeText={(value) => onMutatePolicy({ ...policy, cableSpoof: value })}
          />
        </View>
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
    gap: 18,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    gap: 12,
  },
  rowColumn: {
    gap: 10,
  },
  label: {
    color: '#d9e7fb',
    flex: 1,
    lineHeight: 20,
  },
  input: {
    backgroundColor: '#0b1827',
    color: '#ffffff',
    borderRadius: 12,
    paddingHorizontal: 12,
    paddingVertical: 10,
    minWidth: 92,
  },
  largeInput: {
    minHeight: 88,
    textAlignVertical: 'top',
  },
});
