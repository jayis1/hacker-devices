/**
 * screens/RulesScreen.js — Rule script editor
 *
 * Author: jayis1
 * License: MIT
 *
 * Lets the operator build, load, and clear NVMe-Phantom TLP engine rules.
 * Each rule is 32 bytes (see README §7.3).
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, TextInput, StyleSheet, ScrollView, Picker } from 'react-native';

export default function RulesScreen({ ble }) {
  const [matchType, setMatchType] = useState('1');    // 1=opcode
  const [action, setAction] = useState('1');           // 1=capture
  const [opcode, setOpcode] = useState('0x02');        // Read
  const [modifyField, setModifyField] = useState('0');
  const [modifyValue, setModifyValue] = useState('0x00000000');
  const [ruleCount, setRuleCount] = useState(0);

  const buildRule = () => {
    // Build a 32-byte rule from the UI fields
    const rule = new Uint8Array(32);
    rule[0] = parseInt(matchType, 10);
    rule[1] = parseInt(action, 10);
    rule[2] = parseInt(opcode, 16) & 0xFF;
    rule[3] = 0xFF;  // NSID mask = any
    // modify_field at byte 8, modify_value at bytes 24..27
    rule[8] = parseInt(modifyField, 10);
    const mv = parseInt(modifyValue, 16) >>> 0;
    rule[24] = mv & 0xFF; rule[25] = (mv >> 8) & 0xFF;
    rule[26] = (mv >> 16) & 0xFF; rule[27] = (mv >> 24) & 0xFF;
    return rule;
  };

  const handleLoad = async () => {
    try {
      const rule = buildRule();
      await ble.loadRule(rule);
      setRuleCount(ruleCount + 1);
    } catch (e) { alert('Error loading rule: ' + e.message); }
  };

  const handleClear = async () => {
    try {
      await ble.clearRules();
      setRuleCount(0);
    } catch (e) { alert('Error: ' + e.message); }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Rule Engine</Text>
      <Text style={styles.loaded}>Rules loaded: {ruleCount} / 256</Text>

      <Text style={styles.label}>Match Type</Text>
      <Picker selectedValue={matchType} style={styles.picker} onValueChange={setMatchType}>
        <Picker.Item label="Any command" value="0" />
        <Picker.Item label="Opcode match" value="1" />
        <Picker.Item label="LBA range" value="2" />
        <Picker.Item label="NSID match" value="3" />
        <Picker.Item label="Opal (Security Send/Recv)" value="4" />
        <Picker.Item label="PRP target address" value="5" />
      </Picker>

      <Text style={styles.label}>Action</Text>
      <Picker selectedValue={action} style={styles.picker} onValueChange={setAction}>
        <Picker.Item label="Pass (no-op)" value="0" />
        <Picker.Item label="Capture" value="1" />
        <Picker.Item label="Modify" value="2" />
        <Picker.Item label="Inject" value="3" />
        <Picker.Item label="Drop" value="4" />
        <Picker.Item label="Stall" value="5" />
      </Picker>

      <Text style={styles.label}>Opcode (hex)</Text>
      <TextInput style={styles.input} value={opcode} onChangeText={setOpcode}
        placeholder="0x02 (Read), 0x01 (Write), 0x7D (SecSend)..."/>

      <Text style={styles.label}>Modify Field</Text>
      <Picker selectedValue={modifyField} style={styles.picker} onValueChange={setModifyField}>
        <Picker.Item label="Opcode" value="0" />
        <Picker.Item label="NSID" value="1" />
        <Picker.Item label="PRP1" value="2" />
        <Picker.Item label="PRP2" value="3" />
        <Picker.Item label="CDW10 (LBA lo)" value="4" />
        <Picker.Item label="CDW12 (NLB)" value="5" />
      </Picker>

      <Text style={styles.label}>Modify Value (hex)</Text>
      <TextInput style={styles.input} value={modifyValue} onChangeText={setModifyValue}
        placeholder="0x00000000"/>

      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.buttonGreen} onPress={handleLoad}>
          <Text style={styles.buttonText}>Load Rule</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.buttonRed} onPress={handleClear}>
          <Text style={styles.buttonText}>Clear All</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.footer}>Rule format: 32 bytes — see README §7.3 — jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 5 },
  loaded: { color: '#888', fontSize: 13, marginBottom: 15 },
  label: { color: '#aaa', fontSize: 14, marginTop: 10, marginBottom: 5 },
  picker: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, marginBottom: 5 },
  input: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, padding: 10, fontSize: 14, marginBottom: 5, fontFamily: 'monospace' },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 15 },
  buttonGreen: { backgroundColor: '#00AA00', padding: 12, borderRadius: 6, width: '48%', alignItems: 'center' },
  buttonRed: { backgroundColor: '#cc3300', padding: 12, borderRadius: 6, width: '48%', alignItems: 'center' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 15 },
});