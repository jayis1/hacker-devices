/**
 * ACOUSTIC-PHANTOM — Live Event Feed Screen
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Displays a real-time stream of classified acoustic events.
 * Each event card shows timestamp, class label, confidence, and
 * inter-event timing. Events are color-coded by type.
 */

import React, { useRef, useEffect } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
} from 'react-native';
import { useBLE } from '../utils/ble';
import EventCard from '../components/EventCard';

export default function EventFeedScreen() {
  const ble = useBLE();
  const flatListRef = useRef(null);

  // Auto-scroll to bottom on new events
  useEffect(() => {
    if (ble.events.length > 0 && flatListRef.current) {
      flatListRef.current.scrollToEnd({ animated: true });
    }
  }, [ble.events.length]);

  const renderItem = ({ item, index }) => (
    <EventCard event={item} index={index} />
  );

  // Build reconstructed text from keyboard events
  const reconstructText = () => {
    return ble.events
      .filter(e => e.label && e.confidence > 0.5)
      .map(e => {
        if (e.label === 'SPACE') return ' ';
        if (e.label === 'ENTER') return '\n';
        if (e.label === 'BACKSPACE') return '\b';
        if (e.label === 'TAB') return '\t';
        if (e.label && e.label.length === 1) return e.label;
        return '';
      })
      .join('');
  };

  const reconstructed = reconstructText();

  return (
    <View style={styles.container}>
      {/* Reconstructed text preview */}
      {reconstructed.length > 0 && (
        <View style={styles.previewBar}>
          <Text style={styles.previewLabel}>Reconstructed:</Text>
          <Text style={styles.previewText} numberOfLines={2}>
            {reconstructed}
          </Text>
        </View>
      )}

      {/* Event count + clear button */}
      <View style={styles.headerBar}>
        <Text style={styles.headerText}>
          {ble.events.length} events
        </Text>
        {ble.events.length > 0 && (
          <TouchableOpacity onPress={ble.clearEvents}>
            <Text style={styles.clearButton}>Clear</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Event list */}
      {ble.events.length === 0 ? (
        <View style={styles.emptyState}>
          <Text style={styles.emptyText}>No events detected yet</Text>
          <Text style={styles.emptySubtext}>
            Connect to device and arm capture to begin
          </Text>
        </View>
      ) : (
        <FlatList
          ref={flatListRef}
          data={ble.events}
          renderItem={renderItem}
          keyExtractor={(item, index) => `${item.timestamp}-${index}`}
          contentContainerStyle={styles.list}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a1a' },
  previewBar: {
    padding: 12,
    backgroundColor: '#222',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  previewLabel: {
    color: '#888',
    fontSize: 11,
    textTransform: 'uppercase',
    marginBottom: 4,
  },
  previewText: {
    color: '#4CAF50',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  headerBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  headerText: { color: '#CCC', fontSize: 14, fontWeight: 'bold' },
  clearButton: { color: '#F44336', fontSize: 14 },
  emptyState: { flex: 1, justifyContent: 'center', alignItems: 'center' },
  emptyText: { color: '#555', fontSize: 18, marginBottom: 8 },
  emptySubtext: { color: '#333', fontSize: 14 },
  list: { padding: 8 },
});