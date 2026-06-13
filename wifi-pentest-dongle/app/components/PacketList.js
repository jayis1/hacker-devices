/**
 * WFP-100 Packet List Component
 *
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';

export default function PacketList({ packets }) {
  const getFrameTypeColor = (type) => {
    if (type.startsWith('Beacon') || type.startsWith('Probe')) return '#0ea5e9';
    if (type.startsWith('Auth') || type.startsWith('Assoc')) return '#22c55e';
    if (type.startsWith('Deauth') || type.startsWith('Disassoc')) return '#ef4444';
    if (type === 'Data') return '#f59e0b';
    if (type === 'Ctrl') return '#8b5cf6';
    return '#888';
  };

  const renderPacket = ({ item }) => (
    <View style={styles.packetRow}>
      <Text style={styles.packetId}>#{item.id}</Text>
      <Text style={[styles.packetType, { color: getFrameTypeColor(item.type) }]}>
        {item.type}
      </Text>
      <Text style={styles.packetDetails}>
        Ch {item.channel} | {item.rssi} dBm | {item.len} B
      </Text>
    </View>
  );

  return (
    <FlatList
      data={packets}
      keyExtractor={(item) => item.id.toString()}
      renderItem={renderPacket}
      style={styles.list}
    />
  );
}

const styles = StyleSheet.create({
  list: {
    flex: 1,
  },
  packetRow: {
    flexDirection: 'row',
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderBottomWidth: 0.5,
    borderBottomColor: '#1a1a2e',
    alignItems: 'center',
  },
  packetId: {
    color: '#555',
    fontSize: 10,
    width: 50,
    fontFamily: 'monospace',
  },
  packetType: {
    fontSize: 12,
    fontWeight: '600',
    width: 90,
  },
  packetDetails: {
    color: '#666',
    fontSize: 11,
    flex: 1,
    fontFamily: 'monospace',
  },
});