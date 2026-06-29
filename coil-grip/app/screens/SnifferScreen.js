/*
 * SnifferScreen.js — Passive Qi packet capture viewer
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, FlatList } from 'react-native';
import { Card, Title, Paragraph, Button } from 'react-native-paper';
import { useDevice } from '../components/DeviceContext';

export default function SnifferScreen() {
  const { connected, sendCommand, log } = useDevice();
  const [capture, setCapture] = useState(false);

  const start = () => { setCapture(true); sendCommand('mode sniffer\n'); };
  const stop = () => { setCapture(false); sendCommand('mode idle\n'); sendCommand('log close\n'); };

  // Filter the global log for rx entries (raw packets)
  const packets = log.filter(l => l.type === 'rx').map((l, i) => ({
    id: i, time: new Date(l.t).toISOString().substr(11,8), data: l.msg,
  }));

  const renderItem = ({ item }) => (
    <View style={styles.row}>
      <Text style={styles.idx}>#{item.id}</Text>
      <Text style={styles.time}>{item.time}</Text>
      <Text style={styles.data}>{item.data}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Qi Sniffer</Title>
          <Paragraph>Passive capture of an adjacent Qi link. Place CoilGrip near the pad.</Paragraph>
          <Paragraph>Capturing: {capture ? 'yes' : 'no'} · {connected ? 'online' : 'offline'}</Paragraph>
          <View style={styles.row}>
            <Button mode="contained" onPress={start}>Start capture</Button>
            <Button mode="outlined" onPress={stop}>Stop & save</Button>
          </View>
        </Card.Content>
      </Card>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Captured packets ({packets.length})</Title>
          <FlatList
            data={packets.slice(-40).reverse()}
            keyExtractor={i => i.id.toString()}
            renderItem={renderItem}
            style={styles.list}
          />
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex:1, padding:12, backgroundColor:'#0d1117' },
  card: { marginBottom:12, backgroundColor:'#161b22' },
  row: { flexDirection:'row', alignItems:'center', marginVertical:6, gap:8 },
  list: { maxHeight: 300 },
  idx: { color:'#8b949e', fontFamily:'monospace', fontSize:11, width:40 },
  time: { color:'#3fb950', fontFamily:'monospace', fontSize:11, width:80 },
  data: { color:'#58a6ff', fontFamily:'monospace', fontSize:11, flex:1 },
});