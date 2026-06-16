/**
 * RulesScreen.js — Rules Engine Configuration
 * Author: jayis1
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet,
  Modal, TextInput, Switch, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import RuleCard from '../components/RuleCard';
import HexEditor from '../components/HexEditor';
import { getRules, addRule, deleteRule } from '../services/api';

const RULE_ACTIONS = [
  { value: 0, label: 'Monitor', color: '#48f' },
  { value: 1, label: 'Drop', color: '#f44' },
  { value: 2, label: 'Corrupt', color: '#fa4' },
  { value: 3, label: 'Inject', color: '#f0f' },
];

const DIRECTIONS = [
  { value: 0, label: 'Read (Drive→Host)' },
  { value: 1, label: 'Write (Host→Drive)' },
  { value: 255, label: 'Both' },
];

const RulesScreen = () => {
  const [rules, setRules] = useState([]);
  const [modalVisible, setModalVisible] = useState(false);
  const [editingRule, setEditingRule] = useState(null);

  const [formData, setFormData] = useState({
    lbaStart: '0',
    lbaEnd: '0',
    opcode: '00',
    direction: 255,
    action: 0,
    priority: 5,
    enabled: true,
    name: '',
  });

  const loadRules = useCallback(async () => {
    try {
      const fetched = await getRules();
      setRules(fetched || []);
    } catch (e) {
      console.error('Load rules error:', e);
    }
  }, []);

  React.useEffect(() => { loadRules(); }, [loadRules]);

  const resetForm = () => {
    setFormData({
      lbaStart: '0', lbaEnd: '0', opcode: '00',
      direction: 255, action: 0, priority: 5, enabled: true, name: '',
    });
    setEditingRule(null);
  };

  const handleSave = async () => {
    try {
      const rule = {
        lbaStart: parseInt(formData.lbaStart) || 0,
        lbaEnd: parseInt(formData.lbaEnd) || 0,
        opcode: parseInt(formData.opcode, 16) || 0,
        direction: formData.direction,
        action: formData.action,
        priority: formData.priority,
        enabled: formData.enabled,
        name: formData.name || 'Unnamed Rule',
      };
      await addRule(rule);
      setModalVisible(false);
      resetForm();
      loadRules();
      Alert.alert('Rule Added', `Rule "${rule.name}" has been deployed to the device.`);
    } catch (e) {
      Alert.alert('Error', 'Failed to add rule: ' + e.message);
    }
  };

  const handleDelete = async (index) => {
    try {
      await deleteRule(index);
      loadRules();
    } catch (e) {
      console.error('Delete rule error:', e);
    }
  };

  const renderRuleItem = ({ item, index }) => (
    <RuleCard
      rule={item}
      index={index}
      onDelete={() => handleDelete(index)}
    />
  );

  return (
    <View style={styles.container}>
      <FlatList
        data={rules}
        renderItem={renderRuleItem}
        keyExtractor={(_, i) => String(i)}
        contentContainerStyle={styles.listContent}
        ListHeaderComponent={
          <TouchableOpacity style={styles.addBtn} onPress={() => { resetForm(); setModalVisible(true); }}>
            <Icon name="plus-circle" size={20} color="#fff" />
            <Text style={styles.addBtnText}>Add Rule</Text>
          </TouchableOpacity>
        }
        ListEmptyComponent={
          <View style={styles.emptyState}>
            <Icon name="filter-off" size={48} color="#555" />
            <Text style={styles.emptyText}>No rules configured</Text>
            <Text style={styles.emptySubtext}>Add rules to filter or modify SATA traffic</Text>
          </View>
        }
      />

      <Modal visible={modalVisible} animationType="slide" transparent>
        <View style={styles.modalOverlay}>
          <View style={styles.modal}>
            <Text style={styles.modalTitle}>{editingRule ? 'Edit Rule' : 'New Rule'}</Text>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>Name</Text>
              <TextInput
                style={styles.formInput}
                value={formData.name}
                onChangeText={(t) => setFormData({ ...formData, name: t })}
                placeholder="Boot Sector Monitor"
                placeholderTextColor="#555"
              />
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>LBA Start</Text>
              <TextInput
                style={styles.formInput}
                value={formData.lbaStart}
                onChangeText={(t) => setFormData({ ...formData, lbaStart: t })}
                keyboardType="numeric"
              />
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>LBA End</Text>
              <TextInput
                style={styles.formInput}
                value={formData.lbaEnd}
                onChangeText={(t) => setFormData({ ...formData, lbaEnd: t })}
                keyboardType="numeric"
              />
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>Opcode (hex)</Text>
              <TextInput
                style={[styles.formInput, { width: 80 }]}
                value={formData.opcode}
                onChangeText={(t) => setFormData({ ...formData, opcode: t })}
                maxLength={2}
              />
              <Text style={styles.formHint}>00 = any opcode</Text>
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>Direction</Text>
              <View style={styles.pickerRow}>
                {DIRECTIONS.map((d) => (
                  <TouchableOpacity
                    key={d.value}
                    style={[styles.pickerBtn, formData.direction === d.value && styles.pickerBtnActive]}
                    onPress={() => setFormData({ ...formData, direction: d.value })}
                  >
                    <Text style={[styles.pickerBtnText, formData.direction === d.value && styles.pickerBtnTextActive]}>
                      {d.label}
                    </Text>
                  </TouchableOpacity>
                ))}
              </View>
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>Action</Text>
              <View style={styles.pickerRow}>
                {RULE_ACTIONS.map((a) => (
                  <TouchableOpacity
                    key={a.value}
                    style={[styles.pickerBtn, formData.action === a.value && { backgroundColor: a.color + '44' }]}
                    onPress={() => setFormData({ ...formData, action: a.value })}
                  >
                    <Text style={[styles.pickerBtnText, formData.action === a.value && { color: a.color }]}>
                      {a.label}
                    </Text>
                  </TouchableOpacity>
                ))}
              </View>
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>Priority</Text>
              <TextInput
                style={[styles.formInput, { width: 60 }]}
                value={String(formData.priority)}
                onChangeText={(t) => setFormData({ ...formData, priority: parseInt(t) || 0 })}
                keyboardType="numeric"
              />
              <Text style={styles.formHint}>0=highest</Text>
            </View>

            <View style={styles.formRow}>
              <Text style={styles.formLabel}>Enabled</Text>
              <Switch
                value={formData.enabled}
                onValueChange={(v) => setFormData({ ...formData, enabled: v })}
                trackColor={{ false: '#333', true: '#00ff8844' }}
                thumbColor={formData.enabled ? '#00ff88' : '#666'}
              />
            </View>

            <View style={styles.modalButtons}>
              <TouchableOpacity style={styles.cancelBtn} onPress={() => { setModalVisible(false); resetForm(); }}>
                <Text style={styles.cancelBtnText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity style={styles.saveBtn} onPress={handleSave}>
                <Text style={styles.saveBtnText}>Deploy Rule</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  listContent: { padding: 12 },
  addBtn: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'center',
    backgroundColor: '#1a3a2a', borderRadius: 10, padding: 14, marginBottom: 12,
  },
  addBtnText: { color: '#fff', fontWeight: 'bold', marginLeft: 8, fontSize: 15 },
  emptyState: { alignItems: 'center', paddingVertical: 60 },
  emptyText: { color: '#888', fontSize: 16, marginTop: 16 },
  emptySubtext: { color: '#555', fontSize: 12, marginTop: 8 },
  modalOverlay: {
    flex: 1, backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'center', padding: 20,
  },
  modal: {
    backgroundColor: '#1a1a3a', borderRadius: 16, padding: 20,
    borderWidth: 1, borderColor: '#2a2a5a',
  },
  modalTitle: { color: '#00ff88', fontSize: 18, fontWeight: 'bold', marginBottom: 16 },
  formRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  formLabel: { color: '#aaa', fontSize: 13, width: 90 },
  formInput: {
    backgroundColor: '#0a0a2a', borderRadius: 6, padding: 8,
    color: '#fff', fontSize: 14, flex: 1, borderWidth: 1, borderColor: '#2a2a5a',
  },
  formHint: { color: '#555', fontSize: 11, marginLeft: 8 },
  pickerRow: { flexDirection: 'row', flex: 1, gap: 4 },
  pickerBtn: {
    flex: 1, padding: 6, borderRadius: 6,
    backgroundColor: '#0a0a2a', alignItems: 'center',
    borderWidth: 1, borderColor: '#2a2a5a',
  },
  pickerBtnActive: { borderColor: '#00ff88', backgroundColor: '#00ff8811' },
  pickerBtnText: { color: '#888', fontSize: 11 },
  pickerBtnTextActive: { color: '#00ff88', fontWeight: 'bold' },
  modalButtons: { flexDirection: 'row', gap: 12, marginTop: 16 },
  cancelBtn: {
    flex: 1, padding: 12, borderRadius: 8,
    backgroundColor: '#333', alignItems: 'center',
  },
  cancelBtnText: { color: '#aaa', fontWeight: 'bold' },
  saveBtn: {
    flex: 2, padding: 12, borderRadius: 8,
    backgroundColor: '#00aa55', alignItems: 'center',
  },
  saveBtnText: { color: '#fff', fontWeight: 'bold' },
});

export default RulesScreen;
