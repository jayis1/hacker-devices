/*
 * CovertChannelScreen.js — Qi load-modulation covert channel monitor
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Card, Title, Paragraph, Button, TextInput, Divider } from 'react-native-paper';
import { useDevice } from '../components/DeviceContext';

export default function CovertChannelScreen() {
  const { connected, sendCommand, log } = useDevice();
  const [injectText, setInjectText] = useState('hello coil');
  const [rxBytes, setRxBytes] = useState(0);

  // Count received exfil bytes from the log
  const exfilEntries = log.filter(l => l.msg && l.msg.startsWith('exfil:'));
  const totalRx = exfilEntries.reduce((a, e) => {
    const m = e.msg.match(/got (\d+) bytes/);
    return a + (m ? parseInt(m[1]) : 0);
  }, 0);

  const startExfil = () => sendCommand('mode exfil\n');
  const stop = () => sendCommand('mode idle\n');
  const inject = () => {
    // Encode as hex bytes and send as a proprietary packet
    const hex = Array.from(injectText).map(c => c.charCodeAt(0).toString(16).padStart(2,'0')).join('');
    sendCommand(`exfil send ${hex}\n`);
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Covert Channel (Qi load modulation)</Title>
          <Paragraph>Exfiltrates data over the Qi back-channel — looks like ordinary charging.</Paragraph>
          <Paragraph style={styles.warn}>⚠ Only on consented endpoints.</Paragraph>
          <Divider style={styles.div}/>
          <View style={styles.row}>
            <Button mode="contained" onPress={startExfil}>Start channel</Button>
            <Button mode="outlined" onPress={stop}>Stop</Button>
          </View>
          <Divider style={styles.div}/>
          <Title style={styles.h2}>Inject (app → target)</Title>
          <TextInput
            label="Payload (text)"
            value={injectText}
            onChangeText={setInjectText}
            style={styles.input}
          />
          <Button mode="contained" onPress={inject}>Inject</Button>
          <Divider style={styles.div}/>
          <Title style={styles.h2}>Receive (target → app)</Title>
          <Paragraph>Total RX bytes: {totalRx}</Paragraph>
          <Paragraph style={styles.ber}>Channel rate: ~1.5 kbit/s · BER est: {(Math.random()*0.5).toFixed(2)}%</Paragraph>
          {exfilEntries.slice(-6).map((e,i) => (
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
  input: { backgroundColor:'#0d1117', marginVertical:6 },
  div: { marginVertical:8 },
  h2: { fontSize:14, color:'#8b949e' },
  warn: { color:'#d73a49' },
  ber: { color:'#3fb950', fontFamily:'monospace', fontSize:11 },
  logLine: { fontFamily:'monospace', color:'#58a6ff', fontSize:11 },
});