/**
 * MitmScreen.js — MITM rule editor for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Allows the operator to add custom spoofing / blocking / glitch rules
 * keyed on SBS/PMBus command codes, view the active rule list, and
 * clear all rules.
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  FlatList, Alert,
} from 'react-native';
import { OP, SBS_NAMES } from '../services/BleService';

const ACTIONS = [
  { name: 'Spoof',  code: 1 },
  { name: 'Block',  code: 2 },
  { name: 'Inject', code: 3 },
  { name: 'Log',    code: 4 },
  { name: 'Glitch', code: 5 },
];

export default function MitmScreen({ rules, sendCommand }) {
  const [cmd, setCmd] = useState('0x0D');
  const [mask, setMask] = useState('0xFF');
  const [action, setAction] = useState(1);
  const [spoof, setSpoof] = useState('64,00');  // 100% in little-endian

  const parseHex = (s) => {
    const v = parseInt(s.replace('0x', ''), 16);
    return isNaN(v) ? 0 : v;
  };

  const parseBytes = (s) =>
    s.split(',').map((b) => parseInt(b.trim(), 16) & 0xFF).filter((b) => !isNaN(b));

  const addRule = () => {
    const cmdVal = parseHex(cmd);
    const maskVal = parseHex(mask);
    const spoofBytes = action === 1 ? parseBytes(spoof) : [];
    sendCommand(OP.RULE_ADD, [cmdVal, maskVal, action, ...spoofBytes]);
    Alert.alert('Rule Sent', `Added ${ACTIONS[action - 1].name} rule for cmd ${cmd}`);
  };

  const clearAll = () => {
    sendCommand(OP.RULE_CLR, []);
    Alert.alert('Cleared', 'All MITM rules removed');
  };

  const refresh = () => sendCommand(OP.RULE_LIST, []);

  const renderRule = ({ item }) => (
    <View style={styles.ruleRow}>
      <Text style={styles.ruleCmd}>{SBS_NAMES[parseHex(item.cmd)] || item.cmd}</Text>
      <Text style={styles.ruleMeta}>{item.action}</Text>
      <Text style={styles.ruleMeta}>mask={item.mask}</Text>
      <Text style={styles.ruleMeta}>spoofLen={item.spoofLen}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Add MITM Rule</Text>

        <Text style={styles.label}>SBS Command (hex)</Text>
        <TextInput style={styles.input} value={cmd} onChangeText={setCmd}
          placeholder="0x0D" placeholderTextColor="#555" />

        <Text style={styles.label}>Match Mask (hex)</Text>
        <TextInput style={styles.input} value={mask} onChangeText={setMask}
          placeholder="0xFF" placeholderTextColor="#555" />

        <Text style={styles.label}>Action</Text>
        <View style={styles.actionRow}>
          {ACTIONS.map((a) => (
            <TouchableOpacity
              key={a.code}
              style={[styles.actionBtn, action === a.code && styles.actionActive]}
              onPress={() => setAction(a.code)}
            >
              <Text style={[styles.actionText, action === a.code && styles.actionTextActive]}>
                {a.name}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        {action === 1 && (
            <>
              <Text style={styles.label}>Spoof Bytes (comma-separated hex, LE)</Text>
              <TextInput style={styles.input} value={spoof} onChangeText={setSpoof}
                placeholder="64,00" placeholderTextColor="#555" />
            </>
        )}

        <TouchableOpacity style={styles.addBtn} onPress={addRule}>
          <Text style={styles.addBtnText}>+ Add Rule</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <View style={styles.listHeader}>
          <Text style={styles.title}>Active Rules ({rules.length})</Text>
          <View style={{ flexDirection: 'row' }}>
            <TouchableOpacity onPress={refresh} style={styles.smallBtn}>
              <Text style={styles.smallBtnText}>Refresh</Text>
            </TouchableOpacity>
            <TouchableOpacity onPress={clearAll} style={[styles.smallBtn, { borderColor: '#f00' }]}>
              <Text style={[styles.smallBtnText, { color: '#f00' }]}>Clear All</Text>
            </TouchableOpacity>
          </View>
        </View>

        {rules.length === 0 ? (
          <Text style={styles.empty}>No active rules. Device is in pass-through mode.</Text>
        ) : (
          <FlatList
            data={rules}
            keyExtractor={(_, i) => String(i)}
            renderItem={renderRule}
            style={{ maxHeight: 300 }}
          />
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  card: { backgroundColor: '#16213e', margin: 10, borderRadius: 10, padding: 15 },
  title: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  label: { color: '#888', fontSize: 12, marginTop: 8, marginBottom: 4 },
  input: {
    backgroundColor: '#1a1a2e', color: '#fff', borderWidth: 1,
    borderColor: '#333', borderRadius: 6, padding: 10, fontSize: 14,
  },
  actionRow: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 4 },
  actionBtn: {
    borderWidth: 1, borderColor: '#333', borderRadius: 6,
    padding: 8, marginRight: 6, marginTop: 4,
  },
  actionActive: { borderColor: '#e94560', backgroundColor: '#e9456022' },
  actionText: { color: '#888', fontSize: 12 },
  actionTextActive: { color: '#e94560', fontWeight: 'bold' },
  addBtn: {
    backgroundColor: '#e94560', borderRadius: 8, padding: 12,
    alignItems: 'center', marginTop: 14,
  },
  addBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  listHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  smallBtn: { borderWidth: 1, borderColor: '#555', borderRadius: 6, padding: 6, marginLeft: 8 },
  smallBtnText: { color: '#aaa', fontSize: 11 },
  ruleRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    padding: 10, borderBottomWidth: 1, borderBottomColor: '#222',
  },
  ruleCmd: { color: '#fff', fontSize: 13, fontWeight: '600', flex: 1 },
  ruleMeta: { color: '#888', fontSize: 11, marginLeft: 8 },
  empty: { color: '#666', fontSize: 13, fontStyle: 'italic', padding: 10 },
});