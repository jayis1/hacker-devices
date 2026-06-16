/**
 * DeviceStatus.js — Status Indicator Component
 * Author: jayis1
 *
 * Displays the current SATA Phantom device status in a compact card.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const MODE_INFO = {
  0: { name: 'Transparent', color: '#4a4', icon: 'eye-off' },
  1: { name: 'Monitor', color: '#48f', icon: 'eye' },
  2: { name: 'Active', color: '#f44', icon: 'flash' },
  3: { name: 'Exfil', color: '#fa4', icon: 'upload-network' },
  4: { name: 'Sleep', color: '#888', icon: 'sleep' },
  5: { name: 'USB Config', color: '#f0f', icon: 'usb' },
};

const DeviceStatus = ({ status }) => {
  const mode = MODE_INFO[status.mode] || MODE_INFO[0];

  return (
    <View style={styles.container}>
      <View style={styles.row}>
        <Icon name={mode.icon} size={24} color={mode.color} />
        <View style={styles.info}>
          <Text style={styles.modeText}>{mode.name}</Text>
          <Text style={styles.linkText}>
            {status.linkUp
              ? `Link UP @ ${status.speed === 2 ? '6.0' : status.speed === 1 ? '3.0' : '1.5'} Gbps`
              : 'Link DOWN'}
          </Text>
        </View>
        <View style={styles.batteryContainer}>
          <Icon
            name={status.battery > 3700 ? 'battery-high' : status.battery > 3500 ? 'battery-medium' : 'battery-low'}
            size={20}
            color={status.battery > 3500 ? '#4a4' : '#f44'}
          />
          <Text style={[styles.batteryText, { color: status.battery > 3500 ? '#4a4' : '#f44' }]}>
            {Math.round(status.battery / 37)}%
          </Text>
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#12122a',
    borderRadius: 12,
    padding: 12,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#2a2a4a',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  info: {
    flex: 1,
    marginLeft: 12,
  },
  modeText: {
    color: '#e0e0e0',
    fontSize: 15,
    fontWeight: 'bold',
  },
  linkText: {
    color: '#888',
    fontSize: 12,
    marginTop: 2,
  },
  batteryContainer: {
    alignItems: 'center',
  },
  batteryText: {
    fontSize: 10,
    marginTop: 2,
  },
});

export default DeviceStatus;
