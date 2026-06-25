/**
 * TwrScreen.tsx — two-way ranging control + live chart.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Button, SegmentedButtons, TextInput } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

export default function TwrScreen(): JSX.Element {
  const [role, setRole]     = useState<'tag' | 'anchor'>('anchor');
  const [channel, setChannel] = useState('9');
  const [sts, setSts]         = useState('dynamic');
  const [distances, setDistances] = useState<number[]>([]);
  const [running, setRunning] = useState(false);
  const scrollRef = useRef<ScrollView>(null);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'range') {
        setDistances(d => [...d.slice(-199), e.distance as number]);
      }
    });
    return () => unsub();
  }, []);

  const start = async () => {
    await ble.sendCmd({ v: 'mode', mode: 'twr', role, channel, sts });
    setRunning(true);
  };
  const stop = async () => {
    await ble.sendCmd({ v: 'mode', mode: 'idle' });
    setRunning(false);
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>Two-Way Ranging</Text>
      <Text style={styles.label}>Role</Text>
      <SegmentedButtons value={role}
        onValueChange={v => setRole(v as 'tag' | 'anchor')}
        buttons={[
          { value: 'tag',    label: 'Tag' },
          { value: 'anchor', label: 'Anchor' },
        ]} />
      <Text style={styles.label}>Channel</Text>
      <SegmentedButtons value={channel}
        onValueChange={setChannel}
        buttons={[
          { value: '5', label: 'Ch 5 (6.5 GHz)' },
          { value: '9', label: 'Ch 9 (8 GHz)' },
        ]} />
      <Text style={styles.label}>STS mode</Text>
      <SegmentedButtons value={sts}
        onValueChange={setSts}
        buttons={[
          { value: 'off',     label: 'Off' },
          { value: 'static',  label: 'Static' },
          { value: 'dynamic', label: 'Dynamic' },
        ]} />
      <View style={styles.buttons}>
        <Button mode="contained" onPress={start} disabled={running}>Start</Button>
        <Button mode="outlined"  onPress={stop}  disabled={!running}>Stop</Button>
      </View>
      <Text style={styles.label}>Distance (last {distances.length} samples)</Text>
      <ScrollView ref={scrollRef} style={styles.chart}>
        {distances.map((d, i) => (
          <Text key={i} style={styles.sample}>{i}: {d.toFixed(3)} m</Text>
        ))}
      </ScrollView>
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  label:   { color: '#90a4ae', fontSize: 12, marginTop: 12, marginBottom: 4 },
  buttons: { flexDirection: 'row', gap: 12, marginTop: 16, justifyContent: 'center' },
  chart:   { flex: 1, marginTop: 12, backgroundColor: '#0a0d10' },
  sample:  { color: '#b0bec5', fontSize: 11, fontFamily: 'monospace' },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 8 },
});