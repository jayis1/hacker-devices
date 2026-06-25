/**
 * SettingsScreen.tsx — firmware info, antenna delay, SD format, legal.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, Linking } from 'react-native';
import { Button, TextInput, Card, Divider } from 'react-native-paper';
import { ble, UwbEvent } from '../ble/UwbPhantomBle';

export default function SettingsScreen(): JSX.Element {
  const [fwVersion, setFwVersion] = useState('—');
  const [antDelay, setAntDelay]   = useState('16385');
  const [sdFree, setSdFree]       = useState(0);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'info')   { setFwVersion(e.fw); setSdFree(e.sd_free_kb); }
      if (e.v === 'ant_delay') setAntDelay(String(e.delay));
    });
    ble.sendCmd({ v: 'info' });
    return () => unsub();
  }, []);

  const setDelay = async () => {
    await ble.sendCmd({ v: 'ant_delay', delay: parseInt(antDelay, 10) });
  };

  const formatSd = async () => {
    await ble.sendCmd({ v: 'sd_format' });
  };

  return (
    <View style={styles.root}>
      <Text style={styles.title}>Settings</Text>
      <Card style={styles.card}>
        <Card.Title title="Firmware" />
        <Card.Content>
          <Text style={styles.line}>Version: {fwVersion}</Text>
          <Text style={lines('Author')}>Author: jayis1</Text>
          <Text style={lines('License')}>License: GPL-3.0-or-later</Text>
          <Text style={styles.line}>SD free: {sdFree} KB</Text>
        </Card.Content>
      </Card>
      <Card style={styles.card}>
        <Card.Title title="Antenna delay (DTU)" />
        <Card.Content>
          <TextInput value={antDelay} onChangeText={setAntDelay}
            keyboardType="numeric" style={styles.input} />
          <Button mode="contained" onPress={setDelay} style={styles.btn}>Set</Button>
        </Card.Content>
      </Card>
      <Card style={styles.card}>
        <Card.Title title="Storage" />
        <Card.Content>
          <Button mode="outlined" onPress={formatSd} style={styles.btn}>Format SD card</Button>
        </Card.Content>
      </Card>
      <Divider />
      <Text style={styles.legal}>
        ⚠ Authorized security research only. Unauthorized interception or
        relay of UWB ranging may violate CFAA, ECPA, FCC Part 15, ETSI EN 302 065,
        and motor-vehicle theft law. The author (jayis1) assumes no liability
        for misuse.
      </Text>
      <Text style={styles.link}
        onPress={() => Linking.openURL('https://hermes-agent.nousresearch.com')}>
        Documentation
      </Text>
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
  function lines(_: string) { return styles.line; }
}

const styles = StyleSheet.create({
  root:    { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:   { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  card:    { marginBottom: 12, backgroundColor: '#1a2128' },
  line:    { color: '#b0bec5', fontSize: 13, marginBottom: 4 },
  input:   { backgroundColor: '#0a0d10', marginBottom: 8 },
  btn:     { marginTop: 4 },
  legal:   { color: '#ffab40', fontSize: 10, marginTop: 16, marginBottom: 12, lineHeight: 16 },
  link:    { color: '#42a5f5', fontSize: 12, textAlign: 'center', marginBottom: 8 },
  footer:  { color: '#607d8b', fontSize: 10, textAlign: 'center' },
});