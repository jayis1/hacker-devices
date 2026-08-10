/**
 * LogsScreen.js — Captured PD frame log viewer + export
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity, Share,
} from 'react-native';
import PDMessage from '../components/PDMessage';
import { CMD, RSP } from '../utils/protocol';

export default function LogsScreen() {
  const [frames, setFrames] = useState([]);

  useEffect(() => {
    /* Subscribe to PD_FRAME packets — in a full build this would
     * connect to the shared EmberTapConnection instance. */
  }, []);

  const exportLog = async () => {
    if (frames.length === 0) return;
    const lines = frames.map(f =>
      `${f.ts_ms},${f.sof},0x${f.raw.toString(16).padStart(4,'0')},${f.msg_type},${f.numobj}`
    );
    const csv = 'timestamp_ms,sof,header,msg_type,numobj\n' + lines.join('\n');
    try {
      await Share.share({ message: csv, title: 'ember-tap-pd-log.csv' });
    } catch (e) {
      console.warn(e);
    }
  };

  const renderItem = ({ item }) => <PDMessage frame={item} compact />;

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        <Text style={styles.title}>PD Frame Log</Text>
        <TouchableOpacity onPress={exportLog}>
          <Text style={styles.exportBtn}>Export CSV</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.legend}>
        <Text style={styles.legendText}>
          {frames.length} frames captured
        </Text>
      </View>

      <FlatList
        data={frames}
        renderItem={renderItem}
        keyExtractor={item => item.key}
        ListEmptyComponent={
          <Text style={styles.empty}>
            No frames logged. Use the Sniffer tab to capture PD traffic.
          </Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  toolbar: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    padding: 16, borderBottomWidth: 1, borderBottomColor: '#222',
  },
  title: { color: '#FF6600', fontSize: 18, fontWeight: 'bold' },
  exportBtn: { color: '#FF6600', fontSize: 14 },
  legend: { padding: 8, backgroundColor: '#16213e' },
  legendText: { color: '#888', fontSize: 12 },
  empty: { color: '#666', textAlign: 'center', marginTop: 40, fontSize: 14 },
});

/* end of file — author: jayis1 */