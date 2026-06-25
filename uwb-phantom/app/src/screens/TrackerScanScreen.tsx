/**
 * TrackerScanScreen.tsx — detect and locate UWB emitters following the operator.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';
import { Button } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

interface Emitter { src: number; ch: number; first: number; last: number; count: number; }

export default function TrackerScanScreen(): JSX.Element {
  const [emitters, setEmitters] = useState<Emitter[]>([]);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'track') {
        setEmitters(list => {
          const idx = list.findIndex(x => x.src === e.src);
          if (idx < 0) return [...list, { src: e.src, ch: e.ch, first: Date.now(), last: Date.now(), count: 1 }];
          const copy = [...list];
          copy[idx] = { ...copy[idx], last: Date.now(), count: copy[idx].count + 1 };
          return copy;
        });
      }
      if (e.v === 'track_done') setScanning(false);
    });
    return () => unsub();
  }, []);

  const start = async () => {
    setEmitters([]); setScanning(true);
    await ble.sendCmd({ v: 'mode', mode: 'track', duration: 30 });
  };
  const stop = async () => {
    await ble.sendCmd({ v: 'mode', mode: 'idle' });
    setScanning(false);
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>Tracker Scanner</Text>
      <Text style={styles.desc}>Passive scan for UWB emitters (AirTags, Find My tags, custom anchors) following you.</Text>
      <View style={styles.buttons}>
        <Button mode="contained" onPress={start} loading={scanning} disabled={scanning}>Scan 30s</Button>
        <Button mode="outlined"  onPress={stop}  disabled={!scanning}>Stop</Button>
      </View>
      <Text style={styles.label}>Detected emitters ({emitters.length})</Text>
      <FlatList
        data={emitters.sort((a, b) => b.count - a.count)}
        keyExtractor={i => String(i.src)}
        renderItem={({ item }) => (
          <View style={styles.row}>
            <Text style={styles.src}>src 0x{item.src.toString(16).padStart(2, '0')}</Text>
            <Text style={styles.meta}>ch{item.ch}  seen {item.count}×</Text>
            <Text style={styles.time}>{new Date(item.last).toISOString().slice(11, 19)}</Text>
          </View>
        )}
      />
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  desc:    { color: '#90a4ae', fontSize: 12, marginBottom: 16 },
  buttons: { flexDirection: 'row', gap: 12, justifyContent: 'center', marginBottom: 16 },
  label:   { color: '#90a4ae', fontSize: 12, marginBottom: 8 },
  row:     { flexDirection: 'row', justifyContent: 'space-between',
             paddingVertical: 8, borderBottomWidth: 1, borderBottomColor: '#1c242c' },
  src:     { color: '#e0e0e0', fontSize: 13, fontFamily: 'monospace' },
  meta:    { color: '#b0bec5', fontSize: 11 },
  time:    { color: '#607d8b', fontSize: 11, fontFamily: 'monospace' },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 8 },
});