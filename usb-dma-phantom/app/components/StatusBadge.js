/**
 * StatusBadge — Reusable component showing connection status
 * USB DMA Phantom Companion App
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text, Badge } from 'react-native-paper';

const STATUS_CONFIG = {
  disconnected: { color: '#666666', label: 'DISCONNECTED' },
  connecting: { color: '#ffaa00', label: 'CONNECTING' },
  connected: { color: '#00ff88', label: 'CONNECTED' },
  dma_active: { color: '#ff3366', label: 'DMA ACTIVE' },
  error: { color: '#ff0000', label: 'ERROR' },
};

export default function StatusBadge({ status, size = 'medium' }) {
  const config = STATUS_CONFIG[status] || STATUS_CONFIG.disconnected;
  const badgeSize = size === 'small' ? 8 : size === 'large' ? 16 : 12;

  return (
    <View style={styles.container}>
      <View style={[styles.dot, {
        backgroundColor: config.color,
        width: badgeSize,
        height: badgeSize,
        borderRadius: badgeSize / 2,
      }]} />
      <Text style={[styles.label, { color: config.color, fontSize: size === 'small' ? 10 : 14 }]}>
        {config.label}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 4,
  },
  dot: {
    marginRight: 6,
  },
  label: {
    fontWeight: 'bold',
  },
});