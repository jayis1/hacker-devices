/**
 * SnifferScreen.tsx — UWB passive sniffer control + frame list.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';
import { Button, SegmentedButtons, TextInput } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

interface Frame { ts: number; ch: number; len: number; sts: number; src: number; }

export default function SnifferScreen(): JSX.Element {
  const [channel, setChannel] = useState('9');
  const [duration, setDuration] = useState('30');
  const [frames, setFrames] = useState<Frame[]>([]);
  const [running, setRunning] = useState(false);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'frame') {
        setFrames(f => [...f.slice(-499), {
          ts: Date.now(), ch: e.ch, len: e.len, sts: e.sts, src: e.src }]);
      }
      if (e.v === 'sniff_done') setRunning(false);
    });
    return () => unsub();
  }, []);

  const start = async () => {
    setFrames([]);
    await ble.sendCmd({ v: 'sniff', channel: parseInt(channel, 10),
                       duration: parseInt(duration, 10) });
    setRunning(true);
  };
  const stop = async () => {
    await ble.sendCmd({ v: 'mode', mode: 'idle' });
    setRunning(false);
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>UWB Sniffer (passive RX)</Text>
      <Text style={styles.label}>Channel</Text>
      <SegmentedButtons value={channel} onValueChange={setChannel}
        buttons={[
          { value: '5', label: 'Ch 5' },
          { value: '9', label: 'Ch 9' },
        ]} />
      <Text style={styles.label}>Capture duration (s)</Text>
      <TextInput value={duration} onChangeText={setDuration}
        keyboardType="numeric" style={styles.input} />
      <View style={styles.buttons}>
        <Button mode="contained" onPress={start} disabled={running}>Capture</Button>
        <Button mode="outlined"  onPress={stop}  disabled={!running}>Stop</Button>
      </View>
      <Text style={styles.label}>Frames ({frames.length})</Text>
      <FlatList
        data={frames}
        keyExtractor={(f, i) => String(i)}
        renderItem={({ item }) => (
          <Text style={styles.frame}>
            {new Date(item.ts).toISOString().slice(11, 19)}  ch{item.ch}  src=0x{item.src.toString(16).padStart(2, '0')}  len={item.len}  sts={item.sts}
          </Text>
        )}
      />
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  label:   { color: '#90a4ae', fontSize: 12, marginTop: 12, marginBottom: 4 },
  input:   { backgroundColor: '#1a2128', color: '#fff', marginBottom: 8 },
  buttons: { flexDirection: 'row', gap: 12, marginTop: 16, justifyContent: 'center' },
  frame:   { color: '#b0bec5', fontSize: 10, fontFamily: 'monospace', paddingVertical: 2 },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 8 },
});