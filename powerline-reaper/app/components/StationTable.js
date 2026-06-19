// components/StationTable.js — discovered PLC station table
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';
import { useReaper } from '../utils/protocol';

const ROLE_COLOR = { CCO: '#ff5555', PROXY: '#ffaa00', LEAF: '#00aaff', NONE: '#888' };

export default function StationTable() {
  const { stations } = useReaper();
  return (
    <FlatList
      data={stations}
      keyExtractor={(item, idx) => idx.toString()}
      renderItem={({ item }) => (
        <View style={styles.row}>
          <Text style={[styles.role, { color: ROLE_COLOR[item.role] }]}>{item.role}</Text>
          <Text style={styles.mac}>{item.mac}</Text>
          <Text style={styles.tei}>TEI 0x{item.tei.toString(16).padStart(2,'0')}</Text>
          <Text style={styles.snr}>{item.snr} dB</Text>
          <Text style={styles.nmk}>{item.nmkMatch ? '✓' : '✗'}</Text>
        </View>
      )}
      ListEmptyComponent={<Text style={styles.empty}>No stations discovered</Text>}
    />
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', padding: 8, borderBottomWidth: 0.5, borderBottomColor: '#222', alignItems: 'center' },
  role: { fontSize: 11, fontFamily: 'monospace', width: 60 },
  mac: { color: '#eee', fontSize: 11, fontFamily: 'monospace', width: 150 },
  tei: { color: '#888', fontSize: 10, fontFamily: 'monospace', width: 70 },
  snr: { color: '#ffaa00', fontSize: 10, fontFamily: 'monospace', marginLeft: 'auto' },
  nmk: { color: '#00ffaa', fontSize: 12, marginLeft: 8 },
  empty: { color: '#666', textAlign: 'center', padding: 20 },
});