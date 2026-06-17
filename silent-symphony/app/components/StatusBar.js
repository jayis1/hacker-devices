/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * StatusBar Component — Connection status and device state
 *
 * Shows BLE connection status, device mode, and battery level at a glance.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { MODE_NAMES } from '../utils/protocol';

/**
 * Status bar component showing connection and device state.
 *
 * @param {object} status - DeviceStatus object from protocol.parseStatus()
 */
export default function StatusBar({ status }) {
  if (!status) {
    return (
      <View style={styles.container}>
        <Icon name="bluetooth" size={18} color="#00d4ff" />
        <Text style={styles.connected}>Connected</Text>
        <Text style={styles.waiting}>Waiting for status...</Text>
      </View>
    );
  }

  const batteryColor =
    status.batteryPct < 15
      ? '#ff4444'
      : status.batteryPct < 40
      ? '#ffaa00'
      : '#00c853';

  return (
    <View style={styles.container}>
      <View style={styles.row}>
        <Icon name="bluetooth-connected" size={16} color="#00d4ff" />
        <Text style={styles.connected}>Connected</Text>
      </View>

      <View style={styles.statusRow}>
        <View style={styles.badge}>
          <Icon name="battery-full" size={14} color={batteryColor} />
          <Text style={[styles.badgeText, { color: batteryColor }]}>
            {status.batteryPct}%
          </Text>
        </View>

        <View style={styles.badge}>
          <Icon name="settings-input-component" size={14} color="#7c4dff" />
          <Text style={[styles.badgeText, { color: '#7c4dff' }]}>
            {MODE_NAMES[status.mode] || `Mode ${status.mode}`}
          </Text>
        </View>

        <View style={styles.badge}>
          <Icon name="signal-cellular-alt" size={14} color="#00d4ff" />
          <Text style={[styles.badgeText, { color: '#00d4ff' }]}>
            {status.signalQuality}%
          </Text>
        </View>
      </View>

      <View style={styles.fwRow}>
        <Text style={styles.fwText}>
          FW v{status.fwVersion} · Uptime {Math.floor(status.uptimeS / 60)}m
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 10,
    borderWidth: 1,
    borderColor: '#0d7377',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    marginBottom: 6,
  },
  connected: {
    color: '#00d4ff',
    fontSize: 13,
    fontWeight: '600',
  },
  waiting: {
    color: '#6c757d',
    fontSize: 11,
    fontStyle: 'italic',
  },
  statusRow: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 4,
  },
  badge: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 4,
    paddingHorizontal: 6,
    paddingVertical: 3,
    gap: 4,
  },
  badgeText: {
    fontSize: 11,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  fwRow: {
    alignItems: 'flex-end',
  },
  fwText: {
    color: '#6c757d',
    fontSize: 10,
    fontFamily: 'monospace',
  },
});