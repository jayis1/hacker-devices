/**
 * StatusCard.js — Reusable status display component
 *
 * Displays a labeled card with a colored value, suitable for
 * showing connection status, current mode, RSSI, etc.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

export default function StatusCard({ title, value, color = '#4ecdc4', icon = 'information' }) {
  return (
    <View style={[styles.card, { borderColor: color }]}>
      <View style={styles.iconContainer}>
        <Icon name={icon} size={20} color={color} />
      </View>
      <Text style={styles.title}>{title}</Text>
      <Text style={[styles.value, { color }]} numberOfLines={1} adjustsFontSizeToFit>
        {value}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 10,
    marginHorizontal: 4,
    borderWidth: 1,
    alignItems: 'center',
    minHeight: 72,
    justifyContent: 'center',
  },
  iconContainer: {
    marginBottom: 2,
  },
  title: {
    color: '#888888',
    fontSize: 10,
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  value: {
    fontSize: 13,
    fontWeight: 'bold',
    textAlign: 'center',
    marginTop: 2,
  },
});