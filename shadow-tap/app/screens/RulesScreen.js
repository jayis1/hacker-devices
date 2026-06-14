/**
 * RulesScreen.js — ShadowTap MITM rule management
 * Add, delete, and view packet matching & manipulation rules.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  TextInput, Picker, Alert, Modal
} from 'react-native';

const RULE_TYPES = [
  { label: 'ARP Poison', value: 0x01 },
  { label: 'DNS Spoof', value: 0x02 },
  { label: 'DHCP Rogue', value: 0x03 },
  { label: 'Frame Inject', value: 0x04 },
  { label: 'Drop', value: 0x05 },
  { label: 'Modify', value: 0x06 },
];

export default function RulesScreen({ connected, bleManager }) {
  const [rules, setRules] = useState([]);
  const [modalVisible, setModalVisible] = useState(false);
  const [newRule, setNewRule] = useState({
    type: 0x02,
    description: '',
    matchOffset: 0,
    matchMask: '0xFFFFFFFF',
    matchValue: '0x00000000',
    replaceHex: '',
  });

  const refreshRules = useCallback(async () => {
    if (!connected) return;
    try {
      const list = await bleManager.listRules();
      setRules(list || []);
    } catch (e) {
      console.warn('Failed to fetch rules:', e);
    }
  }, [connected, bleManager]);

  useEffect(() => {
    refreshRules();
  }, [refreshRules]);

  const addRule = async () => {
    if (!connected) return;
    try {
      const payload = buildRulePayload(newRule);
      const result = await bleManager.addRule(payload);
      if (result) {
        Alert.alert('Success', `Rule #${result} added`);
        setModalVisible(false);
        refreshRules();
      }
    } catch (e) {
      Alert.alert('Error', 'Failed to add rule: ' + e.message);
    }
  };

  const deleteRule = async (ruleId) => {
    if (!connected) return;
    Alert.alert(
      'Delete Rule',
      `Remove rule #${ruleId}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            try {
              await bleManager.deleteRule(ruleId);
              refreshRules();
            } catch (e) {
              Alert.alert('Error', 'Failed to delete: ' + e.message);
            }
          },
        },
      ]
    );
  };

  const buildRulePayload = (rule) => {
    // Build binary payload: [type, match_offset(2), match_mask(4), match_value(4), replace_data...]
    const payload = Buffer.alloc(11 + (rule.replaceHex.length / 2));
    payload[0] = rule.type;
    payload.writeUInt16BE(rule.matchOffset, 1);
    payload.writeUInt32BE(parseInt(rule.matchMask, 16), 5);
    payload.writeUInt32BE(parseInt(rule.matchValue, 16), 9);
    // Replace data hex string to bytes
    const hex = rule.replaceHex.replace(/[^0-9a-fA-F]/g, '');
    for (let i = 0; i < hex.length; i += 2) {
      payload[11 + i / 2] = parseInt(hex.substr(i, 2), 16);
    }
    return payload;
  };

  const renderRule = ({ item }) => (
    <View style={styles.ruleCard}>
      <View style={styles.ruleHeader}>
        <Text style={styles.ruleId}>#{item.id}</Text>
        <Text style={styles.ruleType}>
          {RULE_TYPES.find(t => t.value === item.type)?.label || 'Unknown'}
        </Text>
        <TouchableOpacity onPress={() => deleteRule(item.id)}>
          <Text style={styles.deleteText}>✕</Text>
        </TouchableOpacity>
      </View>
      <Text style={styles.ruleDesc}>{item.description || 'No description'}</Text>
      <Text style={styles.ruleDetail}>
        Offset: {item.matchOffset} | Mask: {item.matchMask}
      </Text>
      <View style={[styles.ruleBadge, item.enabled ? styles.badgeOn : styles.badgeOff]}>
        <Text style={styles.badgeText}>{item.enabled ? 'ACTIVE' : 'DISABLED'}</Text>
      </View>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>MITM Rules</Text>
        <TouchableOpacity
          style={styles.addButton}
          onPress={() => setModalVisible(true)}
          disabled={!connected}
        >
          <Text style={styles.addButtonText}>+ Add Rule</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        data={rules}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderRule}
        ListEmptyComponent={
          <Text style={styles.emptyText}>
            {connected ? 'No rules configured' : 'Connect to device first'}
          </Text>
        }
        onRefresh={refreshRules}
        refreshing={false}
      />

      {/* Add Rule Modal */}
      <Modal visible={modalVisible} animationType="slide">
        <View style={styles.modalContainer}>
          <Text style={styles.modalTitle}>New MITM Rule</Text>

          <Text style={styles.label}>Rule Type</Text>
          <Picker
            selectedValue={newRule.type}
            onValueChange={(v) => setNewRule({ ...newRule, type: v })}
            style={styles.picker}
          >
            {RULE_TYPES.map((t) => (
              <Picker.Item key={t.value} label={t.label} value={t.value} />
            ))}
          </Picker>

          <Text style={styles.label}>Description</Text>
          <TextInput
            style={styles.input}
            value={newRule.description}
            onChangeText={(v) => setNewRule({ ...newRule, description: v })}
            placeholder="e.g. Redirect gateway ARP"
            placeholderTextColor="#555"
          />

          <Text style={styles.label}>Match Offset (bytes)</Text>
          <TextInput
            style={styles.input}
            value={String(newRule.matchOffset)}
            onChangeText={(v) => setNewRule({ ...newRule, matchOffset: parseInt(v) || 0 })}
            keyboardType="numeric"
          />

          <Text style={styles.label}>Match Mask (hex)</Text>
          <TextInput
            style={styles.input}
            value={newRule.matchMask}
            onChangeText={(v) => setNewRule({ ...newRule, matchMask: v })}
            placeholder="0xFFFFFFFF"
            placeholderTextColor="#555"
          />

          <Text style={styles.label}>Match Value (hex)</Text>
          <TextInput
            style={styles.input}
            value={newRule.matchValue}
            onChangeText={(v) => setNewRule({ ...newRule, matchValue: v })}
            placeholder="0x00000000"
            placeholderTextColor="#555"
          />

          <Text style={styles.label}>Replace Data (hex)</Text>
          <TextInput
            style={styles.input}
            value={newRule.replaceHex}
            onChangeText={(v) => setNewRule({ ...newRule, replaceHex: v })}
            placeholder="e.g. C0A80101"
            placeholderTextColor="#555"
          />

          <View style={styles.modalButtons}>
            <TouchableOpacity style={styles.cancelButton} onPress={() => setModalVisible(false)}>
              <Text style={styles.cancelText}>Cancel</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.saveButton} onPress={addRule}>
              <Text style={styles.saveText}>Save</Text>
            </TouchableOpacity>
          </View>
        </View>
      </Modal>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a' },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
  },
  title: { fontSize: 24, fontWeight: 'bold', color: '#00ff88' },
  addButton: {
    backgroundColor: '#00aa55',
    borderRadius: 8,
    paddingHorizontal: 16,
    paddingVertical: 8,
  },
  addButtonText: { color: '#fff', fontWeight: 'bold' },
  ruleCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 10,
    padding: 14,
    marginHorizontal: 16,
    marginBottom: 10,
  },
  ruleHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  ruleId: { color: '#00ff88', fontWeight: 'bold', fontSize: 14 },
  ruleType: { color: '#ffaa00', fontSize: 13 },
  deleteText: { color: '#ff4444', fontSize: 18, fontWeight: 'bold' },
  ruleDesc: { color: '#ccc', fontSize: 13, marginBottom: 4 },
  ruleDetail: { color: '#666', fontSize: 11 },
  ruleBadge: {
    alignSelf: 'flex-start',
    borderRadius: 4,
    paddingHorizontal: 8,
    paddingVertical: 2,
    marginTop: 6,
  },
  badgeOn: { backgroundColor: '#00aa55' },
  badgeOff: { backgroundColor: '#555' },
  badgeText: { color: '#fff', fontSize: 10, fontWeight: 'bold' },
  emptyText: { color: '#666', textAlign: 'center', marginTop: 40, fontSize: 16 },
  modalContainer: {
    flex: 1,
    backgroundColor: '#0f0f1a',
    padding: 20,
    paddingTop: 60,
  },
  modalTitle: { fontSize: 22, fontWeight: 'bold', color: '#00ff88', marginBottom: 20 },
  label: { color: '#888', fontSize: 13, marginBottom: 4, marginTop: 10 },
  input: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    color: '#fff',
    fontSize: 14,
  },
  picker: { color: '#fff', backgroundColor: '#1a1a2e' },
  modalButtons: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 30,
  },
  cancelButton: {
    backgroundColor: '#333',
    borderRadius: 8,
    paddingHorizontal: 24,
    paddingVertical: 14,
  },
  cancelText: { color: '#fff', fontWeight: 'bold' },
  saveButton: {
    backgroundColor: '#00aa55',
    borderRadius: 8,
    paddingHorizontal: 24,
    paddingVertical: 14,
  },
  saveText: { color: '#fff', fontWeight: 'bold' },
});