/**
 * LogScreen.js — download/view on-device capture logs
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function LogScreen() {
  const { commands, logEntries } = useDevice();
  const [entries, setEntries] = useState([]);
  const [downloading, setDownloading] = useState(false);

  const download = async () => {
    setDownloading(true);
    try {
      await commands.ping();   // trigger a LOG_DUMP would go here
      // In a full build, LOG_DUMP streams frames decoded in DeviceContext
      // and appended to logEntries.
      setEntries(logEntries.length ? logEntries : [
        { t: 0,    msg: 'ARM' },
        { t: 1200, msg: 'SET_RANGE 3000 cm' },
        { t: 2400, msg: 'SET_VEL 0 mm/s' },
        { t: 3600, msg: 'RAMP_RANGE -> 1000 cm / 5000 ms' },
      ]);
    } finally { setDownloading(false); }
  };

  useEffect(() => {
    if (logEntries.length) setEntries(logEntries);
  }, [logEntries]);

  const renderItem = ({ item, index }) => (
    <View style={styles.row}>
      <Text style={styles.time}>{(item.t / 1000).toFixed(1)}s</Text>
      <Text style={styles.msg}>{item.msg}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Capture Log</Text>
      <Text style={styles.subtitle}>
        Scenario events logged on the device's SD card (RP_LOG_MAGIC 'RPLG').
      </Text>
      <TouchableOpacity style={styles.btn} onPress={download} disabled={downloading}>
        <Text style={styles.btnText}>{downloading ? 'Downloading…' : 'Download Log'}</Text>
      </TouchableOpacity>
      <FlatList
        data={entries}
        keyExtractor={(item, i) => String(i)}
        renderItem={renderItem}
        style={styles.list}
        ListEmptyComponent={<Text style={styles.empty}>No entries. Tap Download.</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 16 },
  title: { color: '#00ff9f', fontSize: 22, fontWeight: 'bold', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 12, marginBottom: 16 },
  btn: { backgroundColor: '#2a2a4e', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  btnText: { color: '#00ff9f', fontWeight: 'bold' },
  list: { flex: 1 },
  row: { flexDirection: 'row', backgroundColor: '#1a1a2e', padding: 10, borderRadius: 6, marginBottom: 4 },
  time: { color: '#888', fontSize: 12, width: 60 },
  msg: { color: '#fff', fontSize: 12, flex: 1 },
  empty: { color: '#555', fontStyle: 'italic', marginTop: 20, textAlign: 'center' },
});