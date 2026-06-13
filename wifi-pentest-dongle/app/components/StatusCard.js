/**
 * WFP-100 Status Card Component
 *
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { MODE_IDLE, MODE_MONITOR, MODE_INJECT, MODE_EVIL_TWIN, BAND_2GHZ, BAND_5GHZ, BAND_6GHZ } from '../utils/protocol';

const MODE_NAMES = {
  [MODE_IDLE]: 'Idle',
  [MODE_MONITOR]: 'Monitor',
  [MODE_INJECT]: 'Inject',
  [MODE_EVIL_TWIN]: 'Evil Twin',
};

const BAND_NAMES = {
  [BAND_2GHZ]: '2.4 GHz',
  [BAND_5GHZ]: '5 GHz',
  [BAND_6GHZ]: '6 GHz',
};

export default function StatusCard({ connected, mode, channel, band, batteryMv, packetsCaptured, packetsInjected, temperature }) {
  const batteryPct = batteryMv > 0 ? Math.min(100, Math.max(0, ((batteryMv - 3100) / (4200 - 3100)) * 100)) : 0;

  return (
    <View style={[styles.card, !connected && styles.cardDisconnected]}>
      <View style={styles.row}>
        <View style={[styles.statusDot, connected ? styles.dotGreen : styles.dotRed]} />
        <Text style={styles.statusText}>{connected ? 'Connected' : 'Disconnected'}</Text>
      </View>

      {connected && (
        <>
          <View style={styles.row}>
            <Text style={styles.label}>Mode:</Text>
            <Text style={styles.value}>{MODE_NAMES[mode] || 'Unknown'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Channel:</Text>
            <Text style={styles.value}>Ch {channel} ({BAND_NAMES[band] || 'Unknown'})</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Battery:</Text>
            <Text style={styles.value}>{batteryPct.toFixed(0)}% ({batteryMv} mV)</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Captured:</Text>
            <Text style={styles.value}>{packetsCaptured.toLocaleString()} packets</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Injected:</Text>
            <Text style={styles.value}>{packetsInjected.toLocaleString()} packets</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Temp:</Text>
            <Text style={styles.value}>{temperature}°C</Text>
          </View>
        </>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    width: '100%',
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#2a2a4a',
  },
  cardDisconnected: {
    borderColor: '#333',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 6,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  dotGreen: { backgroundColor: '#00ff88' },
  dotRed: { backgroundColor: '#ff4444' },
  statusText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  label: {
    color: '#888',
    fontSize: 14,
    width: 90,
  },
  value: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '500',
  },
});