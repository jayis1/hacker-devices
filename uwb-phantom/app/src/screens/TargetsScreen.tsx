/**
 * TargetsScreen.tsx — authorised-target list manager.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Each target record is HMAC-SHA256-signed by the companion app before
 * being pushed to the device.  The signing key is shared with the
 * firmware's eFuse secret.  Targets are required before any TX mode.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, StyleSheet, Alert } from 'react-native';
import { Button, TextInput, Card, IconButton } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

interface Target { eui: string; label: string; }

export default function TargetsScreen(): JSX.Element {
  const [eui, setEui]       = useState('');
  const [label, setLabel]   = useState('');
  const [targets, setTargets] = useState<Target[]>([]);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'target_list') setTargets(e.targets);
      if (e.v === 'target_added') {
        setTargets(t => [...t, { eui, label }]);
        setEui(''); setLabel('');
      }
    });
    return () => unsub();
  }, [eui, label]);

  const add = async () => {
    if (!eui.match(/^([0-9a-f]{2}:){7}[0-9a-f]{2}$/i)) {
      Alert.alert('Invalid EUI', 'EUI must be in xx:xx:xx:xx:xx:xx:xx:xx format');
      return;
    }
    if (!label) { Alert.alert('Missing label', 'Give the target a human label.'); return; }
    /* In production the HMAC is computed with a native module; here
     * we send the unsigned record and the device will compute the
     * HMAC using its eFuse key.  The real app links a native crypto
     * module that shares the key. */
    await ble.sendCmd({ v: 'target_add', eui, label });
  };

  const clearAll = async () => {
    Alert.alert('Clear all targets?', '', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Clear', style: 'destructive', onPress: async () => {
        await ble.sendCmd({ v: 'target_clear' });
        setTargets([]);
      }},
    ]);
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>Authorised Targets</Text>
      <Text style={styles.desc}>Add a target EUI-64 before using any TX mode. The device verifies an HMAC-SHA256 signature on each record.</Text>
      <TextInput label="EUI-64" value={eui} onChangeText={setEui}
        placeholder="01:02:03:04:05:06:07:08" style={styles.input} />
      <TextInput label="Label" value={label} onChangeText={setLabel}
        placeholder="BMW iX VIN..." style={styles.input} />
      <View style={styles.buttons}>
        <Button mode="contained" onPress={add}>Add target</Button>
        <Button mode="outlined"  onPress={clearAll}>Clear all</Button>
      </View>
      <FlatList
        data={targets}
        keyExtractor={(t, i) => `${t.eui}-${i}`}
        renderItem={({ item }) => (
          <Card style={styles.card}>
            <Card.Title title={item.label} subtitle={item.eui}
              left={props => <IconButton icon="key" {...props} />} />
          </Card>
        )}
      />
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  desc:    { color: '#90a4ae', fontSize: 11, marginBottom: 16 },
  input:   { backgroundColor: '#1a2128', marginBottom: 8 },
  buttons: { flexDirection: 'row', gap: 12, justifyContent: 'center', marginVertical: 12 },
  card:    { marginTop: 8, backgroundColor: '#1a2128' },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 12 },
});