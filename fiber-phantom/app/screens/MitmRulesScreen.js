/**
 * MitmRulesScreen.js — FiberPhantom MITM Rules Management
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Create, edit, enable/disable, and delete MITM frame modification rules.
 * Rules are sent to the FPGA rule RAM via BLE C2 protocol.
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  TextInput, Modal, Alert, FlatList, Switch
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { MITM_RULE_TYPE, bytesToHex, hexToBytes } from '../utils/protocol';

const RULE_TYPES = [
  { type: MITM_RULE_TYPE.PASS, name: 'Pass (Log Only)', icon: 'eye' },
  { type: MITM_RULE_TYPE.DROP, name: 'Drop Frame', icon: 'minus-circle' },
  { type: MITM_RULE_TYPE.MODIFY, name: 'Modify Bytes', icon: 'pencil' },
  { type: MITM_RULE_TYPE.INJECT_AFTER, name: 'Inject After', icon: 'arrow-down' },
  { type: MITM_RULE_TYPE.INJECT_BEFORE, name: 'Inject Before', icon: 'arrow-up' },
  { type: MITM_RULE_TYPE.DELAY, name: 'Delay Frame', icon: 'timer' },
];

export default function MitmRulesScreen({ ble, connected, status }) {
  const [rules, setRules] = useState([]);
  const [mitmEnabled, setMitmEnabled] = useState(false);
  const [showEditor, setShowEditor] = useState(false);
  const [editingIndex, setEditingIndex] = useState(-1);
  const [editingRule, setEditingRule] = useState(createEmptyRule());

  const toggleMitm = async (value) => {
    setMitmEnabled(value);
    if (connected) {
      try {
        await ble.enableMitm(value);
        if (value && !status.mode.includes('Bend')) {
          // Also need to enable injection for VCSEL
          await ble.enableInject(true);
        }
      } catch (e) {
        Alert.alert('Error', e.message);
        setMitmEnabled(!value); // Revert
      }
    }
  };

  const openNewRule = () => {
    setEditingIndex(-1);
    setEditingRule(createEmptyRule());
    setShowEditor(true);
  };

  const openEditRule = (index) => {
    setEditingIndex(index);
    setEditingRule({ ...rules[index] });
    setShowEditor(true);
  };

  const saveRule = async () => {
    if (!validateRule(editingRule)) return;

    const newRules = [...rules];
    if (editingIndex >= 0) {
      newRules[editingIndex] = editingRule;
    } else {
      newRules.push(editingRule);
    }
    setRules(newRules);
    setShowEditor(false);

    if (connected) {
      try {
        const index = editingIndex >= 0 ? editingIndex : newRules.length - 1;
        await ble.setRule(index, editingRule);
        Alert.alert('Success', `Rule ${editingIndex >= 0 ? 'updated' : 'created'} at slot ${index}`);
      } catch (e) {
        Alert.alert('Error', 'Failed to send rule: ' + e.message);
      }
    }
  };

  const deleteRule = async (index) => {
    Alert.alert(
      'Delete Rule',
      `Delete rule at slot ${index}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            const newRules = rules.filter((_, i) => i !== index);
            setRules(newRules);
            if (connected) {
              try {
                await ble.clearRule(index);
              } catch (e) {
                console.error(e);
              }
            }
          },
        },
      ]
    );
  };

  const clearAll = async () => {
    Alert.alert(
      'Clear All Rules',
      'Delete all MITM rules from the device?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Clear All',
          style: 'destructive',
          onPress: async () => {
            setRules([]);
            if (connected) {
              try {
                await ble.clearAllRules();
                Alert.alert('Cleared', 'All rules removed from FPGA');
              } catch (e) {
                Alert.alert('Error', e.message);
              }
            }
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {!connected && (
        <View style={styles.warning}>
          <Icon name="alert-circle" size={20} color="#f85149" />
          <Text style={styles.warningText}>Not connected — rules saved locally only</Text>
        </View>
      )}

      {status.mode === 'Bend-Tap' && (
        <View style={styles.warning}>
          <Icon name="information" size={20} color="#d29922" />
          <Text style={styles.warningText}>
            MITM injection requires Inline-Couple or SFP+ mode. Bend-Tap is read-only.
          </Text>
        </View>
      )}

      {/* MITM Engine Toggle */}
      <View style={styles.card}>
        <View style={styles.row}>
          <View style={{ flex: 1 }}>
            <Text style={styles.cardTitle}>MITM Engine</Text>
            <Text style={styles.cardDesc}>
              Enable the FPGA MITM rule engine and VCSEL injection path.
              Requires Inline-Couple or SFP+ mode.
            </Text>
          </View>
          <Switch
            value={mitmEnabled}
            onValueChange={toggleMitm}
            trackColor={{ false: '#30363d', true: '#da3633' }}
            thumbColor={mitmEnabled ? '#f85149' : '#8b949e'}
            disabled={!connected || status.mode === 'Bend-Tap'}
          />
        </View>
      </View>

      {/* Rules List */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Text style={styles.cardTitle}>Frame Modification Rules ({rules.length}/64)</Text>
          <View style={styles.headerButtons}>
            <TouchableOpacity onPress={openNewRule} disabled={rules.length >= 64}>
              <Icon name="plus-circle" size={28} color={rules.length >= 64 ? '#30363d' : '#00ff88'} />
            </TouchableOpacity>
            {rules.length > 0 && (
              <TouchableOpacity onPress={clearAll} style={{ marginLeft: 12 }}>
                <Icon name="trash-can" size={26} color="#f85149" />
              </TouchableOpacity>
            )}
          </View>
        </View>

        {rules.length === 0 ? (
          <View style={styles.emptyState}>
            <Icon name="filter-variant-off" size={48} color="#30363d" />
            <Text style={styles.emptyText}>No rules defined</Text>
            <Text style={styles.emptySubtext}>Tap + to create a frame modification rule</Text>
          </View>
        ) : (
          rules.map((rule, index) => (
            <RuleCard
              key={index}
              rule={rule}
              index={index}
              onEdit={() => openEditRule(index)}
              onDelete={() => deleteRule(index)}
            />
          ))
        )}
      </View>

      {/* Rule Editor Modal */}
      <Modal
        visible={showEditor}
        animationType="slide"
        onRequestClose={() => setShowEditor(false)}
      >
        <RuleEditor
          rule={editingRule}
          onChange={setEditingRule}
          onSave={saveRule}
          onCancel={() => setShowEditor(false)}
          isNew={editingIndex === -1}
        />
      </Modal>
    </ScrollView>
  );
}

// ---- Rule Card Component ----
function RuleCard({ rule, index, onEdit, onDelete }) {
  const ruleType = RULE_TYPES.find(t => t.type === rule.type) || RULE_TYPES[0];
  const typeColor = rule.type === MITM_RULE_TYPE.DROP ? '#f85149' :
                    rule.type === MITM_RULE_TYPE.MODIFY ? '#d29922' : '#58a6ff';

  return (
    <View style={styles.ruleCard}>
      <View style={styles.ruleHeader}>
        <Icon name={ruleType.icon} size={20} color={typeColor} />
        <Text style={styles.ruleType}>{ruleType.name}</Text>
        <Text style={styles.ruleSlot}>Slot {index}</Text>
      </View>
      <Text style={styles.ruleMatch}>
        Match: offset {rule.matchOffset}, {rule.matchLength} bytes
      </Text>
      <Text style={styles.ruleHex}>
        {bytesToHex(rule.matchData.slice(0, rule.matchLength || 8))}
      </Text>
      <View style={styles.ruleActions}>
        <TouchableOpacity onPress={onEdit} style={styles.ruleAction}>
          <Icon name="pencil" size={18} color="#58a6ff" />
          <Text style={styles.actionText}>Edit</Text>
        </TouchableOpacity>
        <TouchableOpacity onPress={onDelete} style={styles.ruleAction}>
          <Icon name="delete" size={18} color="#f85149" />
          <Text style={[styles.actionText, { color: '#f85149' }]}>Delete</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

// ---- Rule Editor Component ----
function RuleEditor({ rule, onChange, onSave, onCancel, isNew }) {
  const updateRule = (field, value) => {
    onChange({ ...rule, [field]: value });
  };

  const updateMatchData = (hexStr) => {
    const bytes = hexToBytes(hexStr.replace(/\s/g, ''));
    updateRule('matchData', bytes);
    updateRule('matchLength', bytes.length);
  };

  const updateReplaceData = (hexStr) => {
    const bytes = hexToBytes(hexStr.replace(/\s/g, ''));
    updateRule('replaceData', bytes);
    updateRule('replaceLength', bytes.length);
  };

  return (
    <ScrollView style={styles.editorContainer}>
      <View style={styles.editorHeader}>
        <Text style={styles.editorTitle}>{isNew ? 'New Rule' : 'Edit Rule'}</Text>
        <TouchableOpacity onPress={onCancel}>
          <Icon name="close" size={28} color="#8b949e" />
        </TouchableOpacity>
      </View>

      {/* Rule Type Selector */}
      <Text style={styles.editorLabel}>Rule Type</Text>
      <View style={styles.typeGrid}>
        {RULE_TYPES.map(t => (
          <TouchableOpacity
            key={t.type}
            style={[
              styles.typeButton,
              rule.type === t.type && styles.typeButtonActive,
            ]}
            onPress={() => updateRule('type', t.type)}
          >
            <Icon name={t.icon} size={22} color={rule.type === t.type ? '#fff' : '#8b949e'} />
            <Text style={[styles.typeName, rule.type === t.type && styles.typeNameActive]}>
              {t.name}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Match Offset */}
      <Text style={styles.editorLabel}>Match Offset (bytes from frame start)</Text>
      <TextInput
        style={styles.input}
        value={String(rule.matchOffset)}
        onChangeText={(v) => updateRule('matchOffset', parseInt(v) || 0)}
        keyboardType="numeric"
      />

      {/* Match Data (hex) */}
      <Text style={styles.editorLabel}>Match Pattern (hex, e.g. "aa bb cc")</Text>
      <TextInput
        style={[styles.input, styles.hexInput]}
        value={bytesToHex(rule.matchData || [])}
        onChangeText={updateMatchData}
        placeholder="e.g. ff ff ff ff for broadcast MAC"
        placeholderTextColor="#484f58"
        autoCapitalize="none"
        autoCorrect={false}
      />
      <Text style={styles.helperText}>
        {rule.matchLength} bytes will be matched at offset {rule.matchOffset}
      </Text>

      {/* Replace Data (for MODIFY type) */}
      {rule.type === MITM_RULE_TYPE.MODIFY && (
        <>
          <Text style={styles.editorLabel}>Replace Offset</Text>
          <TextInput
            style={styles.input}
            value={String(rule.replaceOffset)}
            onChangeText={(v) => updateRule('replaceOffset', parseInt(v) || 0)}
            keyboardType="numeric"
          />
          <Text style={styles.editorLabel}>Replacement Data (hex)</Text>
          <TextInput
            style={[styles.input, styles.hexInput]}
            value={bytesToHex(rule.replaceData || [])}
            onChangeText={updateReplaceData}
            placeholder="e.g. 08 00 for ARP"
            placeholderTextColor="#484f58"
            autoCapitalize="none"
            autoCorrect={false}
          />
        </>
      )}

      {/* Delay (for DELAY type) */}
      {rule.type === MITM_RULE_TYPE.DELAY && (
        <>
          <Text style={styles.editorLabel}>Delay (microseconds)</Text>
          <TextInput
            style={styles.input}
            value={String(rule.delayUs)}
            onChangeText={(v) => updateRule('delayUs', parseInt(v) || 0)}
            keyboardType="numeric"
          />
        </>
      )}

      {/* Enabled toggle */}
      <View style={styles.row}>
        <Text style={styles.editorLabel}>Enabled</Text>
        <Switch
          value={rule.enabled}
          onValueChange={(v) => updateRule('enabled', v)}
          trackColor={{ false: '#30363d', true: '#238636' }}
          thumbColor={rule.enabled ? '#00ff88' : '#8b949e'}
        />
      </View>

      {/* Save / Cancel */}
      <View style={styles.editorButtons}>
        <TouchableOpacity style={[styles.editorBtn, styles.btnDanger]} onPress={onCancel}>
          <Text style={styles.btnText}>Cancel</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.editorBtn, styles.btnSuccess]} onPress={onSave}>
          <Text style={styles.btnText}>Save Rule</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

// ---- Helpers ----
function createEmptyRule() {
  return {
    type: MITM_RULE_TYPE.PASS,
    enabled: true,
    matchOffset: 0,
    matchLength: 0,
    matchData: [],
    matchMask: [],
    replaceOffset: 0,
    replaceData: [],
    replaceLength: 0,
    delayUs: 0,
  };
}

function validateRule(rule) {
  if (!rule.enabled) return true; // Disabled rules are always valid
  if (rule.matchLength === 0) {
    Alert.alert('Invalid Rule', 'Match pattern cannot be empty');
    return false;
  }
  if (rule.matchLength > 32) {
    Alert.alert('Invalid Rule', 'Match pattern cannot exceed 32 bytes');
    return false;
  }
  if (rule.type === MITM_RULE_TYPE.MODIFY && rule.replaceLength === 0) {
    Alert.alert('Invalid Rule', 'Modify rules require replacement data');
    return false;
  }
  return true;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 12 },
  warning: {
    flexDirection: 'row', alignItems: 'center', gap: 8,
    backgroundColor: '#3d1014', borderRadius: 8, padding: 12, marginBottom: 12,
  },
  warningText: { color: '#f85149', fontSize: 13, flex: 1 },
  card: {
    backgroundColor: '#161b22', borderRadius: 12, padding: 16,
    marginBottom: 12, borderWidth: 1, borderColor: '#30363d',
  },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 },
  cardTitle: { color: '#e6edf3', fontSize: 16, fontWeight: 'bold' },
  cardDesc: { color: '#8b949e', fontSize: 12, marginTop: 4 },
  headerButtons: { flexDirection: 'row', alignItems: 'center' },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  emptyState: { alignItems: 'center', paddingVertical: 32 },
  emptyText: { color: '#8b949e', fontSize: 16, marginTop: 8 },
  emptySubtext: { color: '#6e7681', fontSize: 12, marginTop: 4 },
  ruleCard: {
    backgroundColor: '#21262d', borderRadius: 10, padding: 12,
    marginBottom: 8, borderWidth: 1, borderColor: '#30363d',
  },
  ruleHeader: { flexDirection: 'row', alignItems: 'center', gap: 8, marginBottom: 6 },
  ruleType: { color: '#e6edf3', fontSize: 14, fontWeight: 'bold', flex: 1 },
  ruleSlot: { color: '#6e7681', fontSize: 12 },
  ruleMatch: { color: '#8b949e', fontSize: 12, marginBottom: 4 },
  ruleHex: { color: '#58a6ff', fontSize: 12, fontFamily: 'monospace', marginBottom: 8 },
  ruleActions: { flexDirection: 'row', gap: 16 },
  ruleAction: { flexDirection: 'row', alignItems: 'center', gap: 4 },
  actionText: { color: '#58a6ff', fontSize: 13 },
  // Editor
  editorContainer: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  editorHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 16 },
  editorTitle: { color: '#e6edf3', fontSize: 20, fontWeight: 'bold' },
  editorLabel: { color: '#8b949e', fontSize: 13, marginTop: 12, marginBottom: 4 },
  input: {
    backgroundColor: '#21262d', borderRadius: 8, padding: 12,
    color: '#e6edf3', fontSize: 14, borderWidth: 1, borderColor: '#30363d',
  },
  hexInput: { fontFamily: 'monospace' },
  helperText: { color: '#6e7681', fontSize: 11, marginTop: 4 },
  typeGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  typeButton: {
    flexBasis: '48%', backgroundColor: '#21262d', borderRadius: 8,
    padding: 12, alignItems: 'center', borderWidth: 2, borderColor: 'transparent',
  },
  typeButtonActive: { borderColor: '#00ff88', backgroundColor: '#1a3d28' },
  typeName: { color: '#8b949e', fontSize: 11, marginTop: 4, textAlign: 'center' },
  typeNameActive: { color: '#fff' },
  editorButtons: { flexDirection: 'row', gap: 12, marginTop: 24, marginBottom: 40 },
  editorBtn: { flex: 1, borderRadius: 8, padding: 14, alignItems: 'center' },
  btnSuccess: { backgroundColor: '#238636' },
  btnDanger: { backgroundColor: '#da3633' },
  btnText: { color: '#fff', fontSize: 15, fontWeight: 'bold' },
});