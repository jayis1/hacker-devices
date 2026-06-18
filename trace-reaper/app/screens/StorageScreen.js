/**
 * TRACE-REAPER — Session storage screen
 *
 * Lists saved sessions (count comes back in the status notification traces
 * field after CMD.LIST_SESSIONS), lets the operator open one by index, and
 * offers a wipe-all button.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useContext, useEffect, useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, FlatList, Alert, TextInput } from 'react-native';
import { ConnectionContext } from '../utils/protocol';

export default function StorageScreen() {
  const conn = useContext(ConnectionContext);
  const [snap, setSnap] = useState(conn ? conn._snapshot() : null);
  const [count, setCount] = useState(0);
  const [openIdx, setOpenIdx] = useState('0');

  useEffect(() => {
    if (!conn) return;
    return conn.subscribe(() => setSnap(conn._snapshot()));
  }, [conn]);

  const refresh = async () => {
    try { await conn.listSessions(); } catch (e) { Alert.alert('Failed', String(e)); }
  };

  const open = async () => {
    const idx = parseInt(openIdx, 10) || 0;
    try { await conn.openSession(idx); }
    catch (e) { Alert.alert('Open failed', String(e)); }
  };

  const wipe = async () => {
    Alert.alert('Wipe all sessions?', 'This permanently deletes all saved sessions on the device.',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Wipe', style: 'destructive', onPress: async () => {
          try { await conn.wipe(); } catch (e) { Alert.alert('Wipe failed', String(e)); }
        }},
      ]);
  };

  const s = snap || {};
  const items = Array.from({ length: Math.max(count, s.status?.traces || 0) }).slice(0, 64);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Saved Sessions</Text>
      <Text style={styles.subtitle}>Encrypted on-device storage (QSPI NOR + microSD)</Text>

      <View style={styles.row}>
        <Text style={styles.label}>Session count: {s.status?.traces ?? count}</Text>
        <TouchableOpacity style={styles.smallBtn} onPress={refresh}>
          <Text style={styles.smallBtnText}>Refresh</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.openRow}>
        <TextInput style={styles.idxInput} value={openIdx} onChangeText={setOpenIdx} keyboardType="numeric" />
        <TouchableOpacity style={styles.smallBtn} onPress={open}>
          <Text style={styles.smallBtnText}>Open session</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        data={items}
        keyExtractor={(_, i) => String(i)}
        renderItem={({ item, index }) => (
          <View style={styles.item}>
            <Text style={styles.itemText}>#{index}</Text>
            <Text style={styles.itemHint}>tap Open above with this index</Text>
          </View>
        )}
        style={{ marginTop: 12, flex: 1 }}
      />

      <TouchableOpacity style={[styles.smallBtn, { backgroundColor: '#c0392b' }]} onPress={wipe}>
        <Text style={styles.smallBtnText}>Wipe all sessions</Text>
      </TouchableOpacity>

      <Text style={styles.disclaimer}>
        Per-session records are encrypted with a keystream derived from the
        operator passkey. — jayis1
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1622', padding: 16 },
  title: { color: '#00e5ff', fontSize: 20, fontWeight: '700', marginTop: 16 },
  subtitle: { color: '#9fb1c7', fontSize: 12, marginTop: 2 },
  row: { flexDirection: 'row', alignItems: 'center', marginTop: 14 },
  label: { color: '#e6edf3', fontSize: 13, flex: 1 },
  openRow: { flexDirection: 'row', alignItems: 'center', marginTop: 12 },
  idxInput: { backgroundColor: '#13243a', color: '#e6edf3', borderRadius: 6, padding: 8, width: 70, marginRight: 8, fontSize: 13 },
  smallBtn: { backgroundColor: '#1f6feb', padding: 10, borderRadius: 6 },
  smallBtnText: { color: '#fff', fontWeight: '600', fontSize: 12 },
  item: { flexDirection: 'row', padding: 10, backgroundColor: '#13243a', borderRadius: 6, marginBottom: 6 },
  itemText: { color: '#e6edf3', fontFamily: 'monospace', fontSize: 13, width: 60 },
  itemHint: { color: '#5d7088', fontSize: 11 },
  disclaimer: { color: '#5d7088', fontSize: 10, marginTop: 18, lineHeight: 14 },
});