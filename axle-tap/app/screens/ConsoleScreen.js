// ConsoleScreen.js — raw CLI over BLE
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useState, useRef, useEffect } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import * as Ble from '../utils/ble';
import * as Proto from '../utils/protocol';

export default function ConsoleScreen() {
  const [history, setHistory] = useState([
    'AxleTap CLI — jayis1',
    'Type "help" for commands.',
  ]);
  const [input, setInput] = useState('');
  const scrollRef = useRef(null);

  useEffect(() => {
    if (scrollRef.current) scrollRef.current.scrollToEnd({ animated: true });
  }, [history]);

  const send = async () => {
    if (!input) return;
    const cmd = input;
    setHistory((h) => [...h, `axle> ${cmd}`]);
    setInput('');
    const frame = Proto.encodeFrame(Proto.MSG.CMD, strToBytes(cmd), null);
    const ok = await Ble.sendCommand(frame);
    if (!ok) setHistory((h) => [...h, 'ERROR: BLE not connected']);
    // Responses come back as LOG telemetry; in a full build the response
    // would be appended here. For now we send a status request to fetch.
    if (cmd === 'status' || cmd === 'help') {
      // Response is captured by the device manager
    }
  };

  return (
    <View style={styles.container}>
      <ScrollView ref={scrollRef} style={styles.output}>
        {history.map((line, i) => (
          <Text key={i} style={styles.line}>{line}</Text>
        ))}
      </ScrollView>
      <View style={styles.inputRow}>
        <Text style={styles.prompt}>axle&gt;</Text>
        <TextInput
          style={styles.input}
          value={input}
          onChangeText={setInput}
          onSubmitEditing={send}
          placeholder="type a command"
          placeholderTextColor="#666"
        />
        <TouchableOpacity style={styles.sendBtn} onPress={send}>
          <Text style={styles.sendText}>Send</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

function strToBytes(s) {
  const a = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) a[i] = s.charCodeAt(i);
  return a;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d0d' },
  output: { flex: 1, padding: 10 },
  line: {
    color: '#cdd', fontFamily: 'monospace', fontSize: 11, marginVertical: 1,
  },
  inputRow: {
    flexDirection: 'row', alignItems: 'center',
    borderTopWidth: 1, borderTopColor: '#333', padding: 8,
    backgroundColor: '#1a1a1a',
  },
  prompt: { color: '#7ab8ff', fontFamily: 'monospace', marginRight: 6 },
  input: {
    flex: 1, color: '#eee', fontFamily: 'monospace',
    borderWidth: 1, borderColor: '#333', borderRadius: 4,
    paddingHorizontal: 8, paddingVertical: 4,
  },
  sendBtn: {
    backgroundColor: '#2a4d6e', paddingHorizontal: 12, paddingVertical: 8,
    borderRadius: 4, marginLeft: 8,
  },
  sendText: { color: '#fff', fontSize: 12 },
});