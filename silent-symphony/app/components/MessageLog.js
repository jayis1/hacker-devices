/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * MessageLog Component — Scrollable message history
 *
 * Displays a list of recent messages/captures with timestamps
 * and decoded content.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';

/**
 * Message log — shows recent captured/transmitted messages.
 *
 * @param {object[]} messages - Array of message objects with {id, timestamp, modulation, data, quality}
 * @param {string}   title    - Section title
 */
export default function MessageLog({ messages, title = 'Message Log' }) {
  if (!messages || messages.length === 0) {
    return null;
  }

  const formatTime = (ts) => {
    const d = new Date(ts);
    return `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}:${d.getSeconds().toString().padStart(2, '0')}`;
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>{title}</Text>
      <Text style={styles.subtitle}>
        {messages.length} message{messages.length !== 1 ? 's' : ''}
      </Text>

      <ScrollView style={styles.scrollList} nestedScrollEnabled>
        {messages.map((msg, idx) => (
          <View key={msg.id || idx} style={styles.messageItem}>
            <View style={styles.header}>
              <View style={styles.badgeRow}>
                <View
                  style={[
                    styles.modBadge,
                    {
                      backgroundColor:
                        msg.modulation === 'FSK'
                          ? '#0d7377'
                          : msg.modulation === 'OOK'
                          ? '#7c4dff'
                          : '#ffaa00',
                    },
                  ]}
                >
                  <Text style={styles.modText}>{msg.modulation || 'RAW'}</Text>
                </View>
                <Text style={styles.timestamp}>{formatTime(msg.timestamp)}</Text>
              </View>
              {msg.quality !== undefined && (
                <Text
                  style={[
                    styles.quality,
                    { color: msg.quality > 70 ? '#00c853' : '#ffaa00' },
                  ]}
                >
                  {msg.quality}%
                </Text>
              )}
            </View>
            <Text style={styles.data} numberOfLines={2}>
              {msg.data || `${msg.length} bytes`}
            </Text>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#16213e',
  },
  title: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
    marginBottom: 2,
  },
  subtitle: {
    color: '#6c757d',
    fontSize: 11,
    marginBottom: 10,
  },
  scrollList: {
    maxHeight: 200,
  },
  messageItem: {
    backgroundColor: '#16213e',
    borderRadius: 6,
    padding: 10,
    marginBottom: 6,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 4,
  },
  badgeRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  modBadge: {
    borderRadius: 4,
    paddingHorizontal: 5,
    paddingVertical: 1,
  },
  modText: {
    color: '#fff',
    fontSize: 9,
    fontWeight: 'bold',
  },
  timestamp: {
    color: '#6c757d',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  quality: {
    fontSize: 11,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  data: {
    color: '#b0b0b0',
    fontSize: 11,
    fontFamily: 'monospace',
  },
});