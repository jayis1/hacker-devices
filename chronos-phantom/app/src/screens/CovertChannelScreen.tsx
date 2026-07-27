// src/screens/CovertChannelScreen.tsx — Covert channel send/receive
//
// Author: jayis1
// License: GPL-2.0

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, ScrollView, TextInput, TouchableOpacity, FlatList } from 'react-native';
import { useBle } from '../ble/BleManager';
import { CMD, EVT, CovertMessage } from '../types';
import { encodeCovertData, bytesToHex } from '../ble/protocol';

export default function CovertChannelScreen() {
  const { connected, sendCommand, onEvent } = useBle();
  const [messages, setMessages] = useState<CovertMessage[]>([]);
  const [inputText, setInputText] = useState('');
  const [receiving, setReceiving] = useState(false);
  const msgIdRef = useRef(0);

  useEffect(() => {
    if (!connected) return;
    const unsub = onEvent((frame) => {
      if (frame.opcode === EVT.COVERT_RX) {
        const hex = bytesToHex(frame.payload);
        const decoded = new TextDecoder().decode(frame.payload);
        setMessages((prev) => [{
          timestamp: Date.now(),
          data: hex,
          decoded,
          direction: 'rx' as const,
        }, ...prev].slice(0, 100));
      }
    });
    return unsub;
  }, [connected, onEvent]);

  const send = async () => {
    if (!inputText.trim()) return;
    if (!connected) return;
    const data = encodeCovertData(inputText);
    await sendCommand(CMD.COVERT_SEND, data);
    setMessages((prev) => [{
      timestamp: Date.now(),
      data: bytesToHex(data),
      decoded: inputText,
      direction: 'tx' as const,
    }, ...prev].slice(0, 100));
    setInputText('');
  };

  const toggleReceive = async () => {
    if (receiving) {
      await sendCommand(CMD.COVERT_RECV_STOP, new Uint8Array(0));
      setReceiving(false);
    } else {
      await sendCommand(CMD.COVERT_RECV_START, new Uint8Array(0));
      setReceiving(true);
    }
  };

  const renderMessage = ({ item }: { item: CovertMessage }) => (
    <View style={[styles.msgCard, item.direction === 'tx' ? styles.msgTx : styles.msgRx]}>
      <View style={styles.msgHeader}>
        <Text style={styles.msgDir}>{item.direction === 'tx' ? '📤 TX' : '📥 RX'}</Text>
        <Text style={styles.msgTime}>{new Date(item.timestamp).toLocaleTimeString()}</Text>
      </View>
      <Text style={styles.msgDecoded}>{item.decoded}</Text>
      <Text style={styles.msgHex}>{item.data}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Covert Channel</Text>
      <Text style={styles.subtitle}>
        Exfiltrate data over PTP correctionField or NTP root_delay.
        ~8 bits per frame at 1 frame/sec = 8 bps.
      </Text>

      <View style={styles.controls}>
        <TouchableOpacity
          style={[styles.recvButton, receiving ? styles.recvActive : styles.recvIdle]}
          onPress={toggleReceive}
        >
          <Text style={styles.recvText}>{receiving ? '⏹ Stop RX' : '▶ Start RX'}</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={inputText}
          placeholder="Data to exfiltrate..."
          placeholderTextColor="#444"
          onChangeText={setInputText}
          autoCapitalize="none"
          autoCorrect={false}
        />
        <TouchableOpacity style={styles.sendButton} onPress={send}>
          <Text style={styles.sendText}>Send</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        data={messages}
        renderItem={renderMessage}
        keyExtractor={(item, idx) => `${item.timestamp}-${idx}`}
        contentContainerStyle={{ padding: 16, paddingBottom: 40 }}
        style={{ flex: 1 }}
        inverted
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No covert messages yet.</Text>
            <Text style={styles.emptyHint}>Send data or start receiving to see messages.</Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 16 },
  title: { fontSize: 20, fontWeight: 'bold', color: '#00ff88' },
  subtitle: { fontSize: 11, color: '#666', marginTop: 4, marginBottom: 16 },
  controls: { flexDirection: 'row', marginBottom: 12 },
  recvButton: { paddingHorizontal: 20, paddingVertical: 10, borderRadius: 8 },
  recvIdle: { backgroundColor: '#333' },
  recvActive: { backgroundColor: '#aa3300' },
  recvText: { color: 'white', fontSize: 13, fontWeight: '600' },
  inputRow: { flexDirection: 'row', marginBottom: 16 },
  input: {
    flex: 1, backgroundColor: '#1a1a1a', borderRadius: 8, paddingHorizontal: 12,
    paddingVertical: 10, color: '#ccc', fontSize: 13, borderWidth: 1, borderColor: '#333',
  },
  sendButton: { backgroundColor: '#00aa44', paddingHorizontal: 20, paddingVertical: 10,
    borderRadius: 8, marginLeft: 8, justifyContent: 'center' },
  sendText: { color: 'white', fontWeight: '600', fontSize: 13 },
  msgCard: { backgroundColor: '#1a1a1a', borderRadius: 8, padding: 12, marginBottom: 8 },
  msgTx: { borderLeftWidth: 3, borderLeftColor: '#00ff88' },
  msgRx: { borderLeftWidth: 3, borderLeftColor: '#0088ff' },
  msgHeader: { flexDirection: 'row', justifyContent: 'space-between' },
  msgDir: { fontSize: 10, color: '#666', fontWeight: '600' },
  msgTime: { fontSize: 10, color: '#444' },
  msgDecoded: { fontSize: 13, color: '#ccc', marginTop: 6 },
  msgHex: { fontSize: 9, color: '#666', fontFamily: 'monospace', marginTop: 4 },
  empty: { alignItems: 'center', marginTop: 40 },
  emptyText: { color: '#444', fontSize: 14 },
  emptyHint: { color: '#333', fontSize: 11, marginTop: 4 },
});

// Author: jayis1
// License: GPL-2.0