/**
 * PacketList.js — Displays captured Sub-GHz packets in a scrollable list
 */

import React from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity } from 'react-native';
import { protocolNames, protocolColors } from '../utils/protocol';

export default function PacketList({ packets, onReplay, style }) {
  const renderItem = ({ item, index }) => {
    const protocolColor = protocolColors[item.protocol] || '#757575';
    const protocolLabel = protocolNames[item.protocol] || item.protocol;

    return (
      <TouchableOpacity
        style={styles.packetItem}
        onPress={() => onReplay && onReplay(item)}
        disabled={!onReplay}
      >
        <View style={styles.packetHeader}>
          <Text style={styles.packetIndex}>#{packets.length - index}</Text>
          <View style={[styles.protocolBadge, { backgroundColor: protocolColor }]}>
            <Text style={styles.protocolText}>{protocolLabel}</Text>
          </View>
          <Text style={styles.packetRSSI}>
            {item.rssi} dBm
          </Text>
          <Text style={styles.packetCRC}>
            {item.crcValid ? '✓' : '✗'}
          </Text>
        </View>
        <View style={styles.packetDetails}>
          <Text style={styles.packetTime}>
            {formatTime(item.timestamp)}
          </Text>
          <Text style={styles.packetFreq}>
            {(item.frequency / 1e6).toFixed(1)} MHz
          </Text>
          <Text style={styles.packetLen}>
            {item.length} bytes
          </Text>
        </View>
        <Text style={styles.packetData} numberOfLines={2}>
          {item.rawData}
        </Text>
      </TouchableOpacity>
    );
  };

  return (
    <FlatList
      data={packets}
      renderItem={renderItem}
      keyExtractor={(item) => String(item.id)}
      style={style}
      inverted={true}
      contentContainerStyle={styles.listContent}
    />
  );
}

function formatTime(timestamp) {
  const seconds = timestamp / 1000000; // RAT ticks to seconds (approximate)
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
}

const styles = StyleSheet.create({
  listContent: {
    paddingBottom: 8,
  },
  packetItem: {
    backgroundColor: '#1A1A2E',
    marginHorizontal: 8,
    marginVertical: 2,
    padding: 10,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#2A2A4E',
  },
  packetHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 4,
  },
  packetIndex: {
    color: '#757575',
    fontSize: 11,
    fontWeight: '600',
    marginRight: 8,
  },
  protocolBadge: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 4,
    marginRight: 8,
  },
  protocolText: {
    color: '#FFFFFF',
    fontSize: 10,
    fontWeight: '700',
  },
  packetRSSI: {
    color: '#FFC107',
    fontSize: 12,
    fontWeight: '600',
    marginRight: 8,
    flex: 1,
  },
  packetCRC: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  packetDetails: {
    flexDirection: 'row',
    marginBottom: 4,
  },
  packetTime: {
    color: '#B0BEC5',
    fontSize: 11,
    marginRight: 12,
  },
  packetFreq: {
    color: '#2196F3',
    fontSize: 11,
    marginRight: 12,
  },
  packetLen: {
    color: '#B0BEC5',
    fontSize: 11,
  },
  packetData: {
    color: '#9E9E9E',
    fontSize: 10,
    fontFamily: 'Courier',
    lineHeight: 14,
  },
});