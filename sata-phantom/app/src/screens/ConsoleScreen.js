/**
 * ConsoleScreen.js — Serial Debug Console (over WiFi)
 * Author: jayis1
 */

import React, { useState } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { sendConsoleCommand } from '../services/api';

const ConsoleScreen = () => {
  const [logs, setLogs] = useState([
    { id: Date.now(), text: 'SATA Phantom Console v1.0.0', type: 'system' },
    { id: Date.now() + 1, text: 'Type "help" for available commands', type: 'system' },
  ]);
  const [command, setCommand] = useState('');

  const handleSend = async () => {
    if (!command.trim()) return;

    setLogs(prev => [...prev, { id: Date.now(), text: `> ${command}`, type: 'input' }]);

    try {
      const response = await sendConsoleCommand(command);
      setLogs(prev => [...prev, { id: Date.now(), text: response.output || 'OK', type: 'output' }]);
    } catch (e) {
      setLogs(prev => [...prev, { id: Date.now(), text: `Error: ${e.message}`, type: 'error' }]);
    }

    setCommand('');
  };

  const renderLogItem = ({ item }) => (
    <Text style={[styles.logText, styles[`log_${item.type}`]]}>{item.text}</Text>
  );

  return (
    <View style={styles.container}>
      <FlatList
        data={logs}
        renderItem={renderLogItem}
        keyExtractor={(item) => String(item.id)}
        style={styles.logList}
        contentContainerStyle={styles.logContent}
      />
      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={command}
          onChangeText={setCommand}
          placeholder="Enter command..."
          placeholderTextColor="#444"
          onSubmitEditing={handleSend}
          returnKeyType="send"
        />
        <TouchableOpacity style={styles.sendBtn} onPress={handleSend}>
          <Icon name="send" size={20} color="#fff" />
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  logList: { flex: 1 },
  logContent: { padding: 12 },
  logText: { fontFamily: 'monospace', fontSize: 12, marginBottom: 4, lineHeight: 18 },
  log_system: { color: '#888' },
  log_input: { color: '#00ff88' },
  log_output: { color: '#aaa' },
  log_error: { color: '#f44' },
  inputRow: {
    flexDirection: 'row', padding: 8,
    borderTopWidth: 1, borderTopColor: '#2a2a4a',
    backgroundColor: '#12122a',
  },
  input: {
    flex: 1, backgroundColor: '#0a0a2a', borderRadius: 6, padding: 10,
    color: '#00ff88', fontFamily: 'monospace', fontSize: 13,
    borderWidth: 1, borderColor: '#2a2a5a',
  },
  sendBtn: {
    backgroundColor: '#00aa55', borderRadius: 6, padding: 10,
    marginLeft: 8, justifyContent: 'center',
  },
});

export default ConsoleScreen;
