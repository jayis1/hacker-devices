/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — Console Screen
 *
 * Raw command interface for advanced operators, session log, and
 * exploit script runner.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useRef, useEffect } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, FlatList, Alert } from 'react-native';
import { Buffer } from 'buffer';
import { MessageLog } from '../components/MessageLog';

const COMMANDS = [
  { cmd: 'discover', op: 0x01, desc: 'Auto-discover debug port' },
  { cmd: 'halt', op: 0x04, desc: 'Halt target core' },
  { cmd: 'resume', op: 0x05, desc: 'Resume target core' },
  { cmd: 'status', op: 0x0C, desc: 'Query device status' },
  { cmd: 'lock', op: 0x0D, desc: 'Lock and disconnect' },
  { cmd: 'power_on', op: 0x09, desc: 'Inject target power' },
  { cmd: 'power_off', op: 0x0A, desc: 'Disable target power' },
  { cmd: 'jtag_scan', op: 0x07, desc: 'Scan JTAG chain' },
  { cmd: 'rdp_exploit', op: 0x0B, desc: 'Attempt RDP bypass (DESTRUCTIVE)' },
];

const EXPLOITS = [
  { id: 'stm32f4_rdp1_downgrade', name: 'STM32F4 RDP L1→L0 (erases flash)',
    risk: 'high' },
  { id: 'stm32l0_optbytes', name: 'STM32L0 Option Byte Manipulation',
    risk: 'medium' },
  { id: 'nxp_lpc_pwreplay', name: 'NXP LPC Password Replay',
    risk: 'low' },
  { id: 'generic_fuse_read', name: 'Generic Fuse/eFuse Read',
    risk: 'none' },
];

export default function ConsoleScreen({ logs, sendCommand }) {
  const [input, setInput] = useState('');
  const flatListRef = useRef(null);

  useEffect(() => {
    if (flatListRef.current && logs.length > 0) {
      flatListRef.current.scrollToEnd({ animated: true });
    }
  }, [logs]);

  /**
   * Parse a raw command line and send it.
   * Supports: "discover", "read <hex_addr> <len>",
   *          "write <hex_addr> <hex_data>", "halt", "resume",
   *          "extract <hex_addr> <len>", "status"
   */
  const handleSend = async () => {
    const text = input.trim().toLowerCase();
    if (!text) return;
    const parts = text.split(/\s+/);
    const cmd = parts[0];

    if (cmd === 'read') {
      const addr = parseInt(parts[1] || '0', 16);
      const len = parseInt(parts[2] || '256', 10);
      const payload = Buffer.alloc(8);
      payload.writeUInt32LE(addr, 0);
      payload.writeUInt32LE(len, 4);
      await sendCommand(0x0E, payload);
    } else if (cmd === 'write') {
      const addr = parseInt(parts[1] || '0', 16);
      const data = Buffer.from((parts[2] || '').replace(/0x/g, ''), 'hex');
      const payload = Buffer.alloc(8 + data.length);
      payload.writeUInt32LE(addr, 0);
      payload.writeUInt32LE(data.length, 4);
      data.copy(payload, 8);
      await sendCommand(0x0F, payload);
    } else if (cmd === 'extract') {
      const addr = parseInt(parts[1] || '0x08000000', 16);
      const len = parseInt(parts[2] || '65536', 10);
      const payload = Buffer.alloc(8);
      payload.writeUInt32LE(addr, 0);
      payload.writeUInt32LE(len, 4);
      await sendCommand(0x06, payload);
    } else {
      const match = COMMANDS.find(c => c.cmd === cmd);
      if (match) {
        await sendCommand(match.op, Buffer.alloc(0));
      } else {
        Alert.alert('Unknown command', `Available: ${COMMANDS.map(c => c.cmd).join(', ')}`);
      }
    }
    setInput('');
  };

  const handleExploit = (exploit) => {
    Alert.alert(
      '⚠️ Destructive Action',
      `${exploit.name}\n\nThis may permanently modify or erase the target. Continue?`,
      [
        { text: 'Cancel' },
        { text: 'Execute', style: 'destructive', onPress: () => {
          sendCommand(0x0B, Buffer.from(exploit.id, 'utf8'));
        }}
      ]
    );
  };

  const renderItem = ({ item }) => (
    <View style={styles.logItem}>
      <Text style={styles.logTs}>[{item.ts}]</Text>
      <Text style={[styles.logMsg, item.level === 'error' && styles.logError]}>
        {item.msg}
      </Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Command Console</Text>
        <View style={styles.inputRow}>
          <TextInput style={styles.input} value={input}
                     onChangeText={setInput} placeholder="discover, read 0x08000000 256, ..."
                     placeholderTextColor="#555"
                     onSubmitEditing={handleSend} />
          <TouchableOpacity style={styles.sendBtn} onPress={handleSend}>
            <Text style={styles.sendText}>Send</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.quickCmds}>
          {COMMANDS.slice(0, 5).map(c => (
            <TouchableOpacity key={c.cmd} style={styles.quickBtn}
                              onPress={() => { setInput(c.cmd); }}>
              <Text style={styles.quickText}>{c.cmd}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Exploit Scripts</Text>
        <Text style={styles.desc}>
          Known RDP/CRP bypass and fuse manipulation scripts. Use with
          extreme caution — some are destructive.
        </Text>
        {EXPLOITS.map((e, i) => (
          <TouchableOpacity key={i} style={styles.exploitRow}
                            onPress={() => handleExploit(e)}>
            <View style={[styles.riskDot, { backgroundColor:
              e.risk === 'high' ? '#ff3333' : e.risk === 'medium' ? '#ff9900' :
              e.risk === 'low' ? '#ffcc00' : '#00ff88' }]} />
            <Text style={styles.exploitName}>{e.name}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={[styles.card, { flex: 1 }]}>
        <Text style={styles.cardTitle}>Session Log</Text>
        <FlatList ref={flatListRef} data={logs} renderItem={renderItem}
                  keyExtractor={(item, i) => i.toString()}
                  style={styles.logList} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginBottom: 12,
          borderWidth: 1, borderColor: '#2a2a4e' },
  cardTitle: { color: '#00ff88', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  desc: { color: '#aaa', fontSize: 13, marginBottom: 12, lineHeight: 18 },
  inputRow: { flexDirection: 'row', marginBottom: 8 },
  input: { flex: 1, backgroundColor: '#0f0f1a', color: '#00ff88',
           borderWidth: 1, borderColor: '#2a2a4e', borderRadius: 4,
           padding: 10, fontFamily: 'monospace', fontSize: 13 },
  sendBtn: { backgroundColor: '#00ff88', borderRadius: 4, padding: 10,
             marginLeft: 8, justifyContent: 'center' },
  sendText: { color: '#000', fontWeight: 'bold', fontSize: 13 },
  quickCmds: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  quickBtn: { backgroundColor: '#2a2a4e', borderRadius: 4, padding: 6,
              marginBottom: 4 },
  quickText: { color: '#00ff88', fontSize: 12, fontFamily: 'monospace' },
  exploitRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 10,
               borderBottomWidth: 1, borderBottomColor: '#2a2a4e' },
  riskDot: { width: 10, height: 10, borderRadius: 5, marginRight: 12 },
  exploitName: { color: '#e0e0e0', fontSize: 13, flex: 1 },
  logList: { flex: 1 },
  logItem: { flexDirection: 'row', paddingVertical: 2 },
  logTs: { color: '#555', fontSize: 11, fontFamily: 'monospace' },
  logMsg: { color: '#aaa', fontSize: 12, marginLeft: 6, flex: 1 },
  logError: { color: '#ff5555' },
});