/**
 * DashboardScreen.tsx — live status overview.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Card } from 'react-native-paper';
import { ble, useBle, UwbEvent } from '../ble/UwbPhantomBle';

export default function DashboardScreen(): JSX.Element {
  const { device } = useBle();
  const [mode, setMode]   = useState('IDLE');
  const [batt, setBatt]   = useState(100);
  const [dist, setDist]   = useState(-1);
  const [rssi, setRssi]   = useState(0);
  const [auth, setAuth]   = useState(false);

  useEffect(() => {
    const unsub = ble.onEvent((e: UwbEvent) => {
      if (e.v === 'status')  { setMode(e.mode); setBatt(e.battery); setAuth(e.auth); }
      if (e.v === 'range')   setDist(e.distance);
      if (e.v === 'track')   { setRssi(e.rssi); }
    });
    return () => unsub();
  }, []);

  return (
    <View style={styles.root}>
      <Text style={styles.title}>Dashboard</Text>
      <Card style={styles.card}>
        <Card.Title title={device ? device.name : 'Not connected'} />
        <Card.Content>
          <Text style={styles.line}>Mode:     <Text style={styles.val}>{mode}</Text></Text>
          <Text style={styles.line}>Battery:  <Text style={styles.val}>{batt}%</Text></Text>
          <Text style={styles.line}>Distance: <Text style={styles.val}>{dist < 0 ? '—' : dist.toFixed(2) + ' m'}</Text></Text>
          <Text style={styles.line}>RSSI:     <Text style={styles.val}>{rssi || '—'} dBm</Text></Text>
          <Text style={styles.line}>Auth target: <Text style={auth ? styles.ok : styles.warn}>{auth ? 'LOADED' : 'NONE'}</Text></Text>
        </Card.Content>
      </Card>
      <Text style={styles.footer}>© jayis1 — GPL-3.0-or-later</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  root:   { flex: 1, padding: 16, backgroundColor: '#101418' },
  title:  { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  card:   { marginBottom: 12, backgroundColor: '#1a2128' },
  line:   { color: '#b0bec5', fontSize: 14, marginBottom: 6 },
  val:    { color: '#fff', fontWeight: '600' },
  ok:     { color: '#69f0ae', fontWeight: '700' },
  warn:   { color: '#ffab40', fontWeight: '700' },
  footer: { color: '#607d8b', fontSize: 10, textAlign: 'center', marginTop: 16 },
});