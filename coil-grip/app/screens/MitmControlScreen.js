/*
 * MitmControlScreen.js — Qi negotiation MITM controls
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Switch } from 'react-native';
import { Card, Title, Paragraph, Button, TextInput, Divider } from 'react-native-paper';
import { useDevice } from '../components/DeviceContext';

export default function MitmControlScreen() {
  const { connected, sendCommand, telemetry, log } = useDevice();
  const [powerPct, setPowerPct] = useState('50');
  const [trickle, setTrickle] = useState(false);
  const [replayId, setReplayId] = useState('');

  const startMitm = () => sendCommand('mode mitm\n');
  const stopMitm = () => sendCommand('mode idle\n');
  const setPower = () => sendCommand(`tx power ${powerPct}\n`);
  const toggleTrickle = (v) => {
    setTrickle(v);
    sendCommand(`tx power ${v ? '5' : '50'}\n`);
  };
  const replay = () => sendCommand(`tx start 140000 50\n`); // placeholder: replay captured EPP id
  const resetLink = () => sendCommand('tx stop\n');

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Qi MITM Control</Title>
          <Paragraph>Mode: {telemetry.mode} · {connected ? 'online' : 'offline'}</Paragraph>
          <Divider style={styles.div}/>
          <View style={styles.row}>
            <Button mode="contained" onPress={startMitm}>Start MITM</Button>
            <Button mode="outlined" onPress={stopMitm}>Stop</Button>
          </View>
          <Divider style={styles.div}/>
          <Title style={styles.h2}>Power</Title>
          <View style={styles.row}>
            <TextInput
              label="Power %"
              value={powerPct}
              onChangeText={setPowerPct}
              keyboardType="numeric"
              style={styles.input}
            />
            <Button mode="contained" onPress={setPower}>Set</Button>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Trickle DoS</Text>
            <Switch value={trickle} onValueChange={toggleTrickle}/>
          </View>
          <Divider style={styles.div}/>
          <Title style={styles.h2}>Charger ID replay</Title>
          <TextInput
            label="Captured EPP id (hex)"
            value={replayId}
            onChangeText={setReplayId}
            style={styles.input}
          />
          <Button mode="contained" onPress={replay}>Replay</Button>
          <Divider style={styles.div}/>
          <Title style={styles.h2}>Link reset</Title>
          <Button mode="outlined" onPress={resetLink}>Reset Qi link</Button>
        </Card.Content>
      </Card>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Negotiation log</Title>
          {log.filter(l => l.type==='rx').slice(-8).map((e,i) => (
            <Paragraph key={i} style={styles.logLine}>{e.msg}</Paragraph>
          ))}
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex:1, padding:12, backgroundColor:'#0d1117' },
  card: { marginBottom:12, backgroundColor:'#161b22' },
  row: { flexDirection:'row', alignItems:'center', marginVertical:6, gap:8 },
  input: { flex:1, backgroundColor:'#0d1117', maxWidth:140 },
  label: { color:'#c9d1d9', marginRight:8 },
  div: { marginVertical:8 },
  h2: { fontSize:14, color:'#8b949e' },
  logLine: { fontFamily:'monospace', color:'#58a6ff', fontSize:11 },
});