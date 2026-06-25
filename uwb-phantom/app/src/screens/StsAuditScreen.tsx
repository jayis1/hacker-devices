/**
 * StsAuditScreen.tsx — STS enforcement audit runner.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Button, TextInput, SegmentedButtons, Card } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

type Suite = 'no_sts' | 'static_sts' | 'downgrade' | 'counter_replay';

interface Result {
  suite: Suite; accepted: boolean; quality: number; frames: number;
}

export default function StsAuditScreen(): JSX.Element {
  const [eui, setEui] = useState('01:02:03:04:05:06:07:08');
  const [suite, setSuite] = useState<Suite>('no_sts');
  const [results, setResults] = useState<Result[]>([]);
  const [running, setRunning] = useState(false);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'audit_done') {
        setResults(r => [...r, {
          suite: e.suite, accepted: e.accepted, quality: e.quality, frames: e.frames,
        }]);
        setRunning(false);
      }
    });
    return () => unsub();
  }, []);

  const run = async () => {
    setRunning(true);
    await ble.sendCmd({ v: 'sts_audit', suite, eui });
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>STS Auditor</Text>
      <Text style={styles.label}>Target EUI-64</Text>
      <TextInput value={eui} onChangeText={setEui} style={styles.input} />
      <Text style={styles.label}>Audit suite</Text>
      <SegmentedButtons value={suite} onValueChange={v => setSuite(v as Suite)}
        buttons={[
          { value: 'no_sts',        label: 'No-STS' },
          { value: 'static_sts',    label: 'Static' },
          { value: 'downgrade',     label: 'Downgrade' },
          { value: 'counter_replay', label: 'Replay' },
        ]} />
      <View style={styles.buttons}>
        <Button mode="contained" onPress={run} loading={running} disabled={running}>
          Run audit
        </Button>
      </View>
      {results.map((r, i) => (
        <Card key={i} style={styles.card}>
          <Card.Title title={`Suite: ${r.suite}`} />
          <Card.Content>
            <Text style={r.accepted ? styles.fail : styles.pass}>
              {r.accepted ? '✗ ACCEPTED — verifier has weak STS enforcement' : '✓ rejected — STS enforced'}
            </Text>
            <Text style={styles.meta}>STS quality: {r.quality}, frames: {r.frames}</Text>
          </Card.Content>
        </Card>
      ))}
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  label:   { color: '#90a4ae', fontSize: 12, marginTop: 12, marginBottom: 4 },
  input:   { backgroundColor: '#1a2128', color: '#fff', marginBottom: 8 },
  buttons: { flexDirection: 'row', gap: 12, marginTop: 16 },
  card:    { marginTop: 12, backgroundColor: '#1a2128' },
  pass:    { color: '#69f0ae', fontSize: 13, fontWeight: '600' },
  fail:    { color: '#ff5252', fontSize: 13, fontWeight: '600' },
  meta:    { color: '#90a4ae', fontSize: 11, marginTop: 4 },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 16 },
});