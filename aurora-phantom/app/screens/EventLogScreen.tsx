// screens/EventLogScreen.tsx — raw event capture log + export
// Author: jayis1   License: GPL-2.0
import React from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet, Share } from 'react-native';
import { useStore } from '../src/store';

export default function EventLogScreen() {
  const { log, connected } = useStore();

  const exportLog = async () => {
    const text = log.map(e => `[${e.ts}] ${e.tag}: ${e.msg}`).join('\n');
    try { await Share.share({ message: text }); } catch {}
  };

  return (
    <View style={s.container}>
      <Text style={s.title}>Event Log</Text>
      <TouchableOpacity style={s.btn} onPress={exportLog}>
        <Text style={s.btnText}>Export log</Text>
      </TouchableOpacity>
      <FlatList
        data={log}
        keyExtractor={(item, idx) => String(idx)}
        renderItem={({ item }) => (
          <View style={s.row}>
            <Text style={s.ts}>{new Date(item.ts).toLocaleTimeString()}</Text>
            <Text style={s.tag}>[{item.tag}]</Text>
            <Text style={s.msg}>{item.msg}</Text>
          </View>
        )}
        ListEmptyComponent={<Text style={s.empty}>No events.</Text>}
      />
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#7fd4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 12 },
  btn: { backgroundColor: '#1a3a5c', padding: 12, borderRadius: 8, marginBottom: 12, alignItems: 'center' },
  btnText: { color: '#fff', fontSize: 15, fontWeight: '600' },
  row: { flexDirection: 'row', paddingVertical: 4, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  ts: { color: '#555', fontSize: 11, width: 80 },
  tag: { color: '#7fd4ff', fontSize: 11, width: 70 },
  msg: { color: '#ddd', fontSize: 11, flex: 1 },
  empty: { color: '#555', textAlign: 'center', marginTop: 20 },
});