// lumen-tap/app/screens/RecordScreen.js
// SD-Card session management: start/stop, tag, browse.
//
// Author: jayis1
// Copyright (c) 2026 jayis1 — MIT License

import React, { useState, useContext } from 'react';
import { View, Text, TextInput, StyleSheet, ScrollView, TouchableOpacity, FlatList } from 'react-native';
import { Commands, SD_STATE_LABELS } from '../utils/protocol';
import { DeviceContext } from '../App';

export function RecordScreen() {
  const { send, status } = useContext(DeviceContext);
  const [operator, setOperator]   = useState('');
  const [targetId, setTargetId]   = useState('');
  const [note, setNote]           = useState('');
  const [sessions, setSessions]   = useState([]);
  const sdState = status?.sd_state ?? 0;
  const recording = sdState === 2;

  const startSession = () => {
    if (!operator.trim()) { alert('Operator name required for chain-of-custody'); return; }
    send(Commands.recordStart(operator.trim(), targetId.trim(), note.trim()));
    setSessions(s => [...s, {
      id: Date.now(),
      operator: operator.trim(),
      target: targetId.trim(),
      note: note.trim(),
      start: new Date().toISOString(),
      blocks: 0,
    }]);
  };

  const stopSession = () => {
    send(Commands.recordStop());
    setSessions(s => s.map((x, i) => i === s.length - 1 ? {...x, end: new Date().toISOString(), blocks: status?.sd_blk ?? 0} : x));
  };

  const renderItem = ({item}) => (
    <View style={styles.sessionCard}>
      <Text style={styles.sessionTitle}>#{item.id} — {item.operator}</Text>
      <Text style={styles.sessionMeta}>Target: {item.target || '—'}</Text>
      <Text style={styles.sessionMeta}>Start: {item.start}</Text>
      {item.end ? <Text style={styles.sessionMeta}>End: {item.end}  Blocks: {item.blocks}</Text> : null}
      <Text style={styles.sessionMeta}>Note: {item.note || '—'}</Text>
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.h1}>RECORD</Text>

      <Text style={styles.label}>SD state: <Text style={{color:'#39FF14'}}>{SD_STATE_LABELS[sdState]}</Text>  Blocks: {status?.sd_blk ?? 0}</Text>

      <Text style={styles.section}>SESSION METADATA</Text>
      <TextInput style={styles.input} placeholder="Operator name (required)"
        placeholderTextColor="#555" value={operator} onChangeText={setOperator} />
      <TextInput style={styles.input} placeholder="Target ID / scope ref"
        placeholderTextColor="#555" value={targetId} onChangeText={setTargetId} />
      <TextInput style={[styles.input, {height:80}]} placeholder="Notes (location, glass type, distance...)"
        placeholderTextColor="#555" value={note} onChangeText={setNote}
        multiline={true} />

      <View style={{flexDirection:'row', marginTop: 10}}>
        <TouchableOpacity style={[styles.btn, recording && styles.btnDanger]} onPress={stopSession}
          disabled={!recording}>
          <Text style={styles.btnTxt}>{recording ? 'STOP RECORDING' : 'NOT RECORDING'}</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.btn, styles.btnGo]} onPress={startSession}
          disabled={recording}>
          <Text style={[styles.btnTxt, styles.btnGoTxt]}>START</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.section}>SESSIONS (this app run)</Text>
      <FlatList data={sessions} renderItem={renderItem} keyExtractor={x => String(x.id)}
        scrollEnabled={false} />

      <Text style={styles.foot}>
        Sessions are logged on the device SD card with a header containing
        operator, target, sample rate, and start tick. Pull files via USB
        mass-storage after the engagement.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  h1: { color: '#39FF14', fontSize: 20, fontFamily: 'monospace', marginBottom: 8 },
  section: { color: '#888', fontSize: 11, fontFamily: 'monospace',
             marginTop: 14, marginBottom: 4, borderBottomWidth: 1, borderBottomColor: '#222' },
  label: { color: '#ccc', fontFamily: 'monospace', fontSize: 12, marginVertical: 4 },
  input: { backgroundColor: '#161616', color: '#ccc', borderRadius: 4, padding: 10,
           marginVertical: 4, fontFamily: 'monospace', fontSize: 13, borderWidth: 1, borderColor: '#222' },
  btn: { flex: 1, padding: 12, borderRadius: 4, alignItems: 'center', marginHorizontal: 4,
         backgroundColor: '#222' },
  btnGo: { backgroundColor: '#39FF14' },
  btnDanger: { backgroundColor: '#FF4040' },
  btnTxt: { color: '#39FF14', fontFamily: 'monospace' },
  btnGoTxt: { color: '#000', fontWeight: 'bold' },
  sessionCard: { backgroundColor: '#161616', borderRadius: 4, padding: 10, marginVertical: 4 },
  sessionTitle: { color: '#39FF14', fontFamily: 'monospace', fontSize: 13, marginBottom: 4 },
  sessionMeta: { color: '#888', fontFamily: 'monospace', fontSize: 11 },
  foot: { color: '#555', fontSize: 10, fontFamily: 'monospace', marginTop: 16, fontStyle: 'italic' },
});