/**
 * InjectorScreen.js — SATA Frame Injection Console
 * Author: jayis1
 *
 * Allows the operator to craft and inject raw SATA frames into the bus.
 * Includes a hex editor, pre-built templates, and scheduling controls.
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  ScrollView, Alert, ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import HexEditor from '../components/HexEditor';
import { injectFrame } from '../services/api';

const TEMPLATES = [
  { name: 'Bootloader Redirect', fisType: 0x27, description: 'Redirect read to alternate LBA', size: 20 },
  { name: 'Null Write', fisType: 0x46, description: 'Write all zeros to disk', size: 512 },
  { name: 'TRIM Blocker', fisType: 0x27, description: 'Block TRIM command', size: 20 },
  { name: 'Identity Spoof', fisType: 0x34, description: 'Spoof IDENTIFY response', size: 512 },
  { name: 'CRC Error Trigger', fisType: 0x46, description: 'Force CRC mismatch', size: 4 },
];

const InjectorScreen = () => {
  const [rawData, setRawData] = useState('');
  const [selectedTemplate, setSelectedTemplate] = useState(null);
  const [fisType, setFisType] = useState('27');
  const [injecting, setInjecting] = useState(false);
  const [scheduleEnabled, setScheduleEnabled] = useState(false);
  const [scheduleDelay, setScheduleDelay] = useState('1000');

  const handleInject = async () => {
    if (!rawData.trim()) {
      Alert.alert('No Data', 'Enter hex data or select a template');
      return;
    }

    setInjecting(true);
    try {
      const cleanHex = rawData.replace(/\s+/g, '');
      const ft = parseInt(fisType, 16) || 0x27;
      await injectFrame(cleanHex, ft);
      Alert.alert('Injected', `Frame (FIS 0x${ft.toString(16).padStart(2, '0')}) injected into SATA bus.`);
    } catch (e) {
      Alert.alert('Error', 'Injection failed: ' + e.message);
    } finally {
      setInjecting(false);
    }
  };

  const applyTemplate = (template) => {
    setSelectedTemplate(template);
    setFisType(template.fisType.toString(16).padStart(2, '0'));
    // Set placeholder hex data based on template
    const placeholder = '00'.repeat(Math.min(template.size, 32));
    setRawData(placeholder);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Pre-built Templates</Text>
      <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.templateRow}>
        {TEMPLATES.map((t, i) => (
          <TouchableOpacity
            key={i}
            style={[styles.templateCard, selectedTemplate?.name === t.name && styles.templateCardActive]}
            onPress={() => applyTemplate(t)}
          >
            <Text style={styles.templateName}>{t.name}</Text>
            <Text style={styles.templateDesc}>{t.description}</Text>
            <Text style={styles.templateSize}>{t.size} bytes</Text>
          </TouchableOpacity>
        ))}
      </ScrollView>

      <Text style={styles.sectionTitle}>Frame Data (hex)</Text>
      <HexEditor value={rawData} onChange={setRawData} />

      <View style={styles.configRow}>
        <Text style={styles.configLabel}>FIS Type:</Text>
        <TextInput
          style={styles.hexInput}
          value={fisType}
          onChangeText={setFisType}
          maxLength={2}
        />
        <Text style={styles.configHint}>hex (27=Command, 34=Status, 46=Data)</Text>
      </View>

      <View style={styles.configRow}>
        <Text style={styles.configLabel}>Schedule (ms):</Text>
        <TextInput
          style={styles.numInput}
          value={scheduleDelay}
          onChangeText={setScheduleDelay}
          keyboardType="numeric"
        />
        <TouchableOpacity
          style={[styles.toggleBtn, scheduleEnabled && styles.toggleBtnActive]}
          onPress={() => setScheduleEnabled(!scheduleEnabled)}
        >
          <Text style={[styles.toggleText, scheduleEnabled && styles.toggleTextActive]}>
            {scheduleEnabled ? 'ON' : 'OFF'}
          </Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity
        style={[styles.injectBtn, injecting && styles.injectBtnDisabled]}
        onPress={handleInject}
        disabled={injecting}
      >
        {injecting ? (
          <ActivityIndicator color="#fff" />
        ) : (
          <>
            <Icon name="inject" size={20} color="#fff" />
            <Text style={styles.injectBtnText}>
              {scheduleEnabled ? `SCHEDULE INJECTION (${scheduleDelay}ms)` : 'INJECT FRAME NOW'}
            </Text>
          </>
        )}
      </TouchableOpacity>

      <View style={styles.infoBox}>
        <Icon name="information" size={16} color="#48f" />
        <Text style={styles.infoText}>
          Injected frames are merged into the SATA stream during idle gaps. The host and drive
          will see the injected frame as if it originated from the other side.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a', padding: 12 },
  sectionTitle: { color: '#aaa', fontSize: 13, fontWeight: 'bold', marginTop: 12, marginBottom: 8 },
  templateRow: { marginBottom: 8 },
  templateCard: {
    backgroundColor: '#12122a', borderRadius: 10, padding: 12,
    marginRight: 8, width: 140, borderWidth: 1, borderColor: '#2a2a4a',
  },
  templateCardActive: { borderColor: '#00ff88', backgroundColor: '#00ff8811' },
  templateName: { color: '#00ff88', fontWeight: 'bold', fontSize: 13 },
  templateDesc: { color: '#888', fontSize: 11, marginTop: 4 },
  templateSize: { color: '#555', fontSize: 10, marginTop: 4 },
  configRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 6, gap: 8 },
  configLabel: { color: '#aaa', fontSize: 13, width: 100 },
  hexInput: {
    backgroundColor: '#12122a', borderRadius: 6, padding: 8,
    color: '#fff', fontSize: 14, width: 60, textAlign: 'center',
    borderWidth: 1, borderColor: '#2a2a4a', fontFamily: 'monospace',
  },
  numInput: {
    backgroundColor: '#12122a', borderRadius: 6, padding: 8,
    color: '#fff', fontSize: 14, width: 80, textAlign: 'center',
    borderWidth: 1, borderColor: '#2a2a4a',
  },
  configHint: { color: '#555', fontSize: 10, flex: 1 },
  toggleBtn: {
    paddingHorizontal: 16, paddingVertical: 8, borderRadius: 6,
    backgroundColor: '#331111', borderWidth: 1, borderColor: '#442222',
  },
  toggleBtnActive: { backgroundColor: '#113311', borderColor: '#00ff8844' },
  toggleText: { color: '#f44', fontWeight: 'bold', fontSize: 12 },
  toggleTextActive: { color: '#00ff88' },
  injectBtn: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'center',
    backgroundColor: '#cc0000', borderRadius: 10, padding: 16, marginTop: 12,
  },
  injectBtnDisabled: { backgroundColor: '#441111' },
  injectBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 15, marginLeft: 8 },
  infoBox: {
    flexDirection: 'row', backgroundColor: '#0a1a3a', borderRadius: 8,
    padding: 12, marginTop: 16, gap: 8,
  },
  infoText: { color: '#888', fontSize: 12, flex: 1, lineHeight: 18 },
});

export default InjectorScreen;
