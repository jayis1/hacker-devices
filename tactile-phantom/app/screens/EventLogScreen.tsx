/**
 * Tactile-Phantom — Companion App
 * screens/EventLogScreen.tsx — Searchable event log
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useMemo } from 'react';
import {
  View, Text, FlatList, TextInput, StyleSheet, TouchableOpacity,
} from 'react-native';
import { useStore, EV_TYPE_NAMES } from '../src/store';

export default function EventLogScreen() {
  const events = useStore((s) => s.events);
  const clearEvents = useStore((s) => s.clearEvents);
  const [filter, setFilter] = useState('');
  const [typeFilter, setTypeFilter] = useState<number | null>(null);

  const filteredEvents = useMemo(() => {
    let result = [...events].reverse();
    if (typeFilter !== null) {
      result = result.filter((e) => e.type === typeFilter);
    }
    if (filter.length > 0) {
      const lower = filter.toLowerCase();
      result = result.filter((e) => {
        const text = `${EV_TYPE_NAMES[e.type] || ''} ${e.fingers.map((f) => `(${f.x},${f.y})`).join(' ')}`.toLowerCase();
        return text.includes(lower);
      });
    }
    return result;
  }, [events, filter, typeFilter]);

  const renderItem = ({ item, index }: { item: any; index: number }) => (
    <View style={styles.eventRow}>
      <Text style={styles.eventIndex}>{String(index + 1).padStart(5, '0')}</Text>
      <Text style={[
        styles.eventType,
        { color: typeColor(item.type) },
      ]}>
        {EV_TYPE_NAMES[item.type] || `T${item.type}`}
      </Text>
      <Text style={styles.eventData}>
        {item.fingers.length > 0
          ? item.fingers.map((f) => `(${f.x},${f.y})`).join(' ')
          : item.gestureId !== undefined
          ? `G:${item.gestureId}`
          : item.regAddr !== undefined
          ? `R:0x${item.regAddr.toString(16)}`
          : '-'}
      </Text>
      <Text style={styles.eventTime}>
        {(item.timestamp / 1000000).toFixed(3)}s
      </Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.filterBar}>
        <TextInput
          style={styles.searchInput}
          placeholder="Search events..."
          placeholderTextColor="#555"
          value={filter}
          onChangeText={setFilter}
        />
        <TouchableOpacity style={styles.clearBtn} onPress={clearEvents}>
          <Text style={styles.clearBtnText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.typeFilterRow}>
        <TouchableOpacity
          style={[styles.typeBtn, typeFilter === null && styles.typeBtnActive]}
          onPress={() => setTypeFilter(null)}
        >
          <Text style={styles.typeBtnText}>All</Text>
        </TouchableOpacity>
        {[1, 2, 3, 4, 5].map((t) => (
          <TouchableOpacity
            key={t}
            style={[styles.typeBtn, typeFilter === t && styles.typeBtnActive]}
            onPress={() => setTypeFilter(t)}
          >
            <Text style={styles.typeBtnText}>{EV_TYPE_NAMES[t]}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.headerRow}>
        <Text style={styles.headerText}>#</Text>
        <Text style={styles.headerText}>Type</Text>
        <Text style={styles.headerText}>Data</Text>
        <Text style={styles.headerText}>Time</Text>
      </View>

      <FlatList
        data={filteredEvents}
        keyExtractor={(_, i) => String(i)}
        renderItem={renderItem}
        style={styles.list}
        ListEmptyComponent={
          <Text style={styles.emptyText}>No events match the filter.</Text>
        }
      />

      <View style={styles.footer}>
        <Text style={styles.footerText}>
          {filteredEvents.length} / {events.length} events
        </Text>
      </View>
    </View>
  );
}

function typeColor(type: number): string {
  switch (type) {
    case 1: return '#00ff88';  // TOUCH
    case 2: return '#ff6666';  // RELEASE
    case 3: return '#ffaa00';  // GESTURE
    case 4: return '#66aaff';  // CONFIG
    case 5: return '#ff00ff';  // FW_UPDATE
    case 6: return '#888888';  // BUS_EVENT
    default: return '#aaa';
  }
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a' },
  filterBar: { flexDirection: 'row', padding: 10, gap: 10 },
  searchInput: {
    flex: 1, backgroundColor: '#16213e', borderRadius: 8, padding: 10,
    color: '#e0e0e0', fontSize: 14, borderWidth: 1, borderColor: '#0f3460',
  },
  clearBtn: {
    backgroundColor: '#3a1a1a', borderRadius: 8, padding: 10,
    borderWidth: 1, borderColor: '#cc3333',
  },
  clearBtnText: { color: '#cc8888', fontSize: 13 },
  typeFilterRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 5, paddingHorizontal: 10, marginBottom: 5 },
  typeBtn: {
    backgroundColor: '#16213e', borderRadius: 6, padding: 5, paddingHorizontal: 10,
    borderWidth: 1, borderColor: '#0f3460',
  },
  typeBtnActive: { borderColor: '#00ff88', backgroundColor: '#0a2a1a' },
  typeBtnText: { color: '#888', fontSize: 11 },
  headerRow: {
    flexDirection: 'row', padding: 8, backgroundColor: '#16213e',
    borderBottomWidth: 1, borderBottomColor: '#0f3460',
  },
  headerText: { color: '#666', fontSize: 10, fontWeight: 'bold' },
  list: { flex: 1 },
  eventRow: {
    flexDirection: 'row', padding: 8, borderBottomWidth: 0.5,
    borderBottomColor: '#1a1a2e', alignItems: 'center',
  },
  eventIndex: { color: '#555', fontSize: 10, width: 50 },
  eventType: { fontSize: 11, fontWeight: 'bold', width: 80 },
  eventData: { color: '#aaa', fontSize: 10, flex: 1 },
  eventTime: { color: '#666', fontSize: 10, width: 60 },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 30 },
  footer: { padding: 10, backgroundColor: '#16213e' },
  footerText: { color: '#666', fontSize: 11, textAlign: 'center' },
});