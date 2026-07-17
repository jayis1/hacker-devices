// src/screens/SnifferScreen.tsx — live PD message sniffer timeline.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { useMemo } from 'react';
import { View, Text, FlatList, Pressable, StyleSheet } from 'react-native';
import { useDevice } from '../state/DeviceContext';
import { ConnectionBar } from '../components/ConnectionBar';
import { PdMsg, SOP_NAMES, CTRL_NAMES, DATA_NAMES, parseFixedPdo, PD_DATA } from '../protocol';

function describeMsg(m: PdMsg): string {
  const isCtrl = ((m.header >> 14) & 0x07) === 0;
  const type = m.header & 0x0f;
  const name = isCtrl ? (CTRL_NAMES[type] ?? `Ctrl#${type}`) : (DATA_NAMES[type] ?? `Data#${type}`);
  let detail = '';
  if (!isCtrl && type === PD_DATA.SOURCE_CAP && m.obj.length) {
    detail = m.obj.map((pdo, i) => {
      const p = parseFixedPdo(pdo);
      return `PDO${i}=${p.mv}mV/${p.ma}mA`;
    }).join(' ');
  } else if (!isCtrl && type === PD_DATA.REQUEST && m.obj.length) {
    const rdo = m.obj[0];
    const idx = (rdo >> 28) & 0x07;
    const op = (rdo & 0x3ff) * 10;
    detail = `idx=${idx} op=${op}mA`;
  }
  return `${name}${detail ? '  ' + detail : ''}`;
}

export default function SnifferScreen() {
  const { messages, clearMessages, send, connected } = useDevice();

  const renderItem = ({ item }: { item: PdMsg }) => (
    <View style={[styles.row, { borderLeftColor: item.direction === 'src->snk' ? '#2a9d8f' : '#e76f51' }]}>
      <Text style={styles.dir}>{item.direction ?? '—'}</Text>
      <Text style={styles.sop}>{SOP_NAMES[item.sop] ?? '?'}</Text>
      <Text style={styles.id}>#{item.msg_id}</Text>
      <Text style={styles.body}>{describeMsg(item)}</Text>
    </View>
  );

  const sorted = useMemo(() => [...messages].reverse(), [messages]);

  return (
    <View style={styles.container}>
      <ConnectionBar />
      <View style={styles.toolbar}>
        <Pressable style={styles.btn} onPress={() => send({ cmd: 'mode', mode: 'sniff' })}>
          <Text style={styles.btnText}>Start Sniff</Text>
        </Pressable>
        <Pressable style={styles.btn} onPress={clearMessages}>
          <Text style={styles.btnText}>Clear</Text>
        </Pressable>
        <View style={{ flex: 1 }} />
        <Text style={styles.count}>{messages.length} msgs</Text>
      </View>
      <FlatList
        data={sorted}
        keyExtractor={(_, i) => i.toString()}
        renderItem={renderItem}
        ListEmptyComponent={<Text style={styles.empty}>No PD traffic yet. Connect and press Start Sniff.</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d0d' },
  toolbar:   { flexDirection: 'row', alignItems: 'center', padding: 6, gap: 6 },
  btn:       { backgroundColor: '#333', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 6, marginRight: 6 },
  btnText:   { color: '#eee', fontSize: 12, fontWeight: '600' },
  count:     { color: '#888', fontSize: 12, marginRight: 8 },
  row:       { flexDirection: 'row', alignItems: 'center', padding: 8, borderBottomWidth: 1, borderColor: '#222', borderLeftWidth: 3 },
  dir:       { color: '#4cc9f0', fontSize: 11, width: 70 },
  sop:       { color: '#9b5de5', fontSize: 11, width: 45 },
  id:        { color: '#888', fontSize: 11, width: 40 },
  body:      { color: '#eee', fontSize: 12, flex: 1 },
  empty:     { color: '#666', textAlign: 'center', marginTop: 40, paddingHorizontal: 20 },
});