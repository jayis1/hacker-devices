/**
 * screens/BusMonitor.js — live decoded 1553 word stream + fault counters
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { Card, Row, Label, Value, Button, COLORS } from '../components/Themed';
import { useCdc } from '../utils/cdc';
import { savePcapng } from '../utils/pcapng';

export default function BusMonitor() {
  const { send, onLine } = useCdc();
  const [lines, setLines] = useState([]);
  const [paused, setPaused] = useState(false);
  const [gap, setGap] = useState(0);
  const [parity, setParity] = useState(0);
  const [sync, setSync] = useState(0);
  const linesRef = useRef([]);

  useEffect(() => {
    send('bm start');
    const off = onLine((line) => {
      if (paused) return;
      const m = line.match(/^(\d+),(\d+),(\w+),([0-9A-F]+),([0-9A-F]+)/);
      if (m) {
        const [, ts, ch, type, word, status] = m;
        linesRef.current = [...linesRef.current.slice(-200), { ts, ch, type, word, status }];
        setLines(linesRef.current);
      } else if (line.startsWith('bm:')) {
        const f = line.match(/gap=(\d+)\s+rto=\d+\s+par=(\d+)\s+syn=(\d+)/);
        if (f) { setGap(+f[1]); setParity(+f[2]); setSync(+f[3]); }
      }
    });
    return () => { off(); send('bm stop'); };
  }, [onLine, send, paused]);

  const renderItem = ({ item }) => {
    const color = item.type === 'CMD' ? COLORS.accent
                 : item.type === 'STA' ? COLORS.ok
                 : item.type === 'DAT' ? COLORS.text
                 : COLORS.warn;
    return (
      <View style={styles.row}>
        <Text style={styles.ts}>{item.ts}</Text>
        <Text style={[styles.ch, { color: item.ch === '0' ? COLORS.ok : COLORS.accent }]}>
          {item.ch === '0' ? 'A' : 'B'}
        </Text>
        <Text style={[styles.type, { color }]}>{item.type}</Text>
        <Text style={styles.word}>{item.word}</Text>
      </View>
    );
  };

  const exportPcap = async () => {
    const csv = linesRef.current.map((l) => `${l.ts},${l.ch},${l.type},${l.word},${l.status}`);
    await savePcapng(csv, `1553-phantom-${Date.now()}.pcapng`);
  };

  return (
    <View style={styles.container}>
      <Card>
        <Row>
          <Label>Gap faults</Label><Value color={gap ? COLORS.danger : COLORS.text}>{gap}</Value>
        </Row>
        <Row>
          <Label>Parity faults</Label><Value color={parity ? COLORS.danger : COLORS.text}>{parity}</Value>
        </Row>
        <Row>
          <Label>Sync faults</Label><Value color={sync ? COLORS.danger : COLORS.text}>{sync}</Value>
        </Row>
        <View style={styles.btnRow}>
          <Button title={paused ? 'Resume' : 'Pause'} onPress={() => setPaused((p) => !p)} color={COLORS.warn} />
          <Button title="Flush" onPress={() => send('bm flush')} />
          <Button title="Export .pcapng" onPress={exportPcap} color={COLORS.accent} />
        </View>
      </Card>

      <FlatList
        data={lines}
        keyExtractor={(item, i) => String(i)}
        renderItem={renderItem}
        style={styles.list}
      />
      <Text style={styles.footer}>by jayis1</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: COLORS.bg, padding: 12 },
  btnRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6, marginVertical: 6 },
  list: { flex: 1, marginTop: 8 },
  row: { flexDirection: 'row', paddingVertical: 2, fontFamily: 'monospace' },
  ts: { color: COLORS.muted, width: 80, fontSize: 11 },
  ch: { width: 18, fontWeight: '700', fontSize: 11 },
  type: { width: 40, fontWeight: '700', fontSize: 11 },
  word: { color: COLORS.text, fontFamily: 'monospace', fontSize: 11 },
  footer: { color: COLORS.muted, fontSize: 10, textAlign: 'center', paddingVertical: 8 },
});