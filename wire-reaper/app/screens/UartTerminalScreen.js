/**
 * UartTerminalScreen.js — Bidirectional UART console for debug port access
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, buildUartSniff, buildUartInject, buildUartSetBaud } from '../utils/protocol';

const BAUD_RATES = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1000000];

export default function UartTerminalScreen() {
  const { sendCommand, setStreamHandler } = useDevice();
  const [channel, setChannel] = useState('0');
  const [baud, setBaud] = useState('115200');
  const [inputText, setInputText] = useState('');
  const [terminalLines, setTerminalLines] = useState([]);
  const [autoBaud, setAutoBaud] = useState(false);
  const scrollRef = useRef(null);

  // Stream handler for incoming UART data
  const handleStreamData = useCallback((data) => {
    // data[0] = opcode (0x30), data[1] = channel, data[2] = length, data[3+] = bytes
    if (data.length > 3) {
      const len = data[2];
      const bytes = [];
      for (let i = 3; i < Math.min(3 + len, data.length); i++) {
        bytes.push(data[i]);
      }
      // Convert to string, showing printable chars and hex for non-printable
      const text = bytes.map(b => {
        if (b === 0x0A) return '\n';
        if (b === 0x0D) return '';
        if (b >= 32 && b < 127) return String.fromCharCode(b);
        return `\\x${b.toString(16).padStart(2, '0')}`;
      }).join('');

      if (text.trim()) {
        setTerminalLines(prev => [...prev, { id: Date.now() + Math.random(), text, type: 'rx' }]);
      }
    }
  }, []);

  useEffect(() => {
    setStreamHandler(handleStreamData);
    return () => setStreamHandler(null);
  }, [handleStreamData, setStreamHandler]);

  const handleSetBaud = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const b = parseInt(baud, 10) || 115200;
    await sendCommand(CMD.UART_SET_BAUD, buildUartSetBaud(ch, b));
  }, [channel, baud, sendCommand]);

  const handleSend = useCallback(async () => {
    if (!inputText.trim()) return;
    const ch = parseInt(channel, 10) || 0;
    const bytes = [];
    for (let i = 0; i < inputText.length; i++) {
      bytes.push(inputText.charCodeAt(i) & 0xFF);
    }
    // Add newline if not present
    if (!inputText.endsWith('\n')) {
      bytes.push(0x0A);
    }

    await sendCommand(CMD.UART_INJECT, buildUartInject(ch, bytes));
    setTerminalLines(prev => [...prev, { id: Date.now(), text: inputText, type: 'tx' }]);
    setInputText('');
  }, [channel, inputText, sendCommand]);

  const handleStartSniff = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    await handleSetBaud();
    // Start sniffing (poll mode)
    const resp = await sendCommand(CMD.UART_SNIFF, buildUartSniff(ch));
    if (resp && resp.length > 3) {
      const len = resp[2];
      if (len > 0) {
        const bytes = [];
        for (let i = 3; i < 3 + len; i++) bytes.push(resp[i]);
        const text = bytes.map(b => b >= 32 && b < 127 ? String.fromCharCode(b) : `\\x${b.toString(16).padStart(2, '0')}`).join('');
        setTerminalLines(prev => [...prev, { id: Date.now(), text, type: 'rx' }]);
      }
    }
  }, [channel, handleSetBaud, sendCommand]);

  const handleClear = () => setTerminalLines([]);

  const renderItem = ({ item }) => (
    <Text style={[styles.terminalLine, item.type === 'tx' ? styles.txLine : styles.rxLine]}>
      {item.type === 'tx' ? '> ' : '< '}{item.text}
    </Text>
  );

  return (
    <View style={styles.container}>
      <View style={styles.configRow}>
        <Text style={styles.label}>Channel:</Text>
        <TextInput style={styles.smallInput} value={channel} onChangeText={setChannel} keyboardType="numeric" />
        <Text style={styles.label}>Baud:</Text>
        <TextInput style={styles.smallInput} value={baud} onChangeText={setBaud} keyboardType="numeric" />
        <TouchableOpacity style={styles.smallButton} onPress={handleSetBaud}>
          <Text style={styles.smallButtonText}>Set</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.baudRow}>
        {BAUD_RATES.map(b => (
          <TouchableOpacity
            key={b}
            style={[styles.baudChip, baud === b.toString() && styles.baudActive]}
            onPress={() => setBaud(b.toString())}
          >
            <Text style={styles.baudText}>{b}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.actionRow}>
        <TouchableOpacity style={[styles.button, styles.sniffButton]} onPress={handleStartSniff}>
          <Text style={styles.buttonText}>Poll RX</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.button, styles.clearButton]} onPress={handleClear}>
          <Text style={styles.buttonText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        ref={scrollRef}
        data={terminalLines}
        keyExtractor={item => item.id.toString()}
        renderItem={renderItem}
        style={styles.terminal}
        onContentSizeChange={() => {
          if (scrollRef.current && terminalLines.length > 0) {
            scrollRef.current.scrollToEnd({ animated: true });
          }
        }}
        ListEmptyComponent={<Text style={styles.empty}>UART terminal ready. Send commands below.</Text>}
      />

      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={inputText}
          onChangeText={setInputText}
          placeholder="Type command to send..."
          placeholderTextColor="#555"
          onSubmitEditing={handleSend}
          autoCapitalize="none"
          autoCorrect={false}
        />
        <TouchableOpacity style={styles.sendButton} onPress={handleSend}>
          <Text style={styles.sendText}>Send</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  configRow: { flexDirection: 'row', alignItems: 'center', gap: 8, marginBottom: 8 },
  label: { color: '#888', fontSize: 12 },
  smallInput: { backgroundColor: '#1a1a2e', color: '#fff', padding: 6, borderRadius: 4, width: 70, fontSize: 13, fontFamily: 'monospace' },
  smallButton: { backgroundColor: '#e94560', padding: 6, borderRadius: 4 },
  smallButtonText: { color: '#fff', fontSize: 12 },
  baudRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 4, marginBottom: 8 },
  baudChip: { backgroundColor: '#1a1a2e', borderRadius: 4, padding: 4, paddingHorizontal: 8 },
  baudActive: { backgroundColor: '#e94560' },
  baudText: { color: '#aaa', fontSize: 10, fontFamily: 'monospace' },
  actionRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  button: { padding: 10, borderRadius: 6, flex: 1, alignItems: 'center' },
  sniffButton: { backgroundColor: '#4ecca3' },
  clearButton: { backgroundColor: '#333' },
  buttonText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  terminal: { flex: 1, backgroundColor: '#0a0a14', borderRadius: 8, padding: 8, marginBottom: 8 },
  terminalLine: { fontFamily: 'monospace', fontSize: 12, paddingVertical: 1 },
  txLine: { color: '#e94560' },
  rxLine: { color: '#4ecca3' },
  empty: { color: '#555', textAlign: 'center', padding: 20 },
  inputRow: { flexDirection: 'row', gap: 8 },
  input: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    color: '#fff',
    padding: 10,
    borderRadius: 6,
    fontFamily: 'monospace',
    fontSize: 13,
  },
  sendButton: { backgroundColor: '#e94560', padding: 10, borderRadius: 6, paddingHorizontal: 20 },
  sendText: { color: '#fff', fontWeight: 'bold' },
});