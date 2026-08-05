/**
 * SettingsScreen.js — App settings screen
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: MIT
 *
 * Configure success/failure patterns, trigger source, target UART
 * baud rate, and view device information.
 */

import React, { useState, useContext } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet,
  ScrollView, Switch, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { DeviceContext } from '../utils/deviceContext';
import { CMD_SET_TRIGGER, CMD_SET_PATTERNS } from '../utils/protocol';

const TRIGGER_SOURCES = [
  { value: 0, label: 'GPIO Edge' },
  { value: 1, label: 'UART Word' },
  { value: 2, label: 'Power Envelope' },
  { value: 3, label: 'Manual' },
  { value: 4, label: 'Timer' },
  { value: 5, label: 'External Sync' },
];

export default function SettingsScreen() {
  const { connected, sendCommand } = useContext(DeviceContext);

  const [triggerSource, setTriggerSource] = useState(0);
  const [triggerEdge, setTriggerEdge] = useState(0); // 0 = rising, 1 = falling
  const [uartWord, setUartWord] = useState('');
  const [powerThreshold, setPowerThreshold] = useState('500');
  const [targetBaud, setTargetBaud] = useState('115200');

  const [successPatterns, setSuccessPatterns] = useState(['# ', 'root@', 'debug>']);
  const [failurePatterns, setFailurePatterns] = useState(['Access denied', 'Authentication failed', 'Reset']);

  const handleApplyTrigger = async () => {
    if (!connected) return;

    // Build trigger config payload
    const payload = new Uint8Array(32);
    payload[0] = triggerSource;
    payload[1] = triggerEdge;

    if (triggerSource === 1) { // UART word
      const wordBytes = stringToBytes(uartWord);
      payload[2] = Math.min(wordBytes.length, 16);
      for (let i = 0; i < payload[2]; i++) {
        payload[3 + i] = wordBytes[i];
      }
    } else if (triggerSource === 2) { // Power envelope
      const thr = parseInt(powerThreshold) || 0;
      payload[19] = thr & 0xFF;
      payload[20] = (thr >> 8) & 0xFF;
    }

    await sendCommand(CMD_SET_TRIGGER, payload);
    Alert.alert('Trigger Updated', `Source: ${TRIGGER_SOURCES[triggerSource].label}`);
  };

  const handleApplyPatterns = async () => {
    if (!connected) return;

    // Build patterns payload (simplified)
    const payload = new Uint8Array(256);
    let offset = 0;

    // Success patterns
    payload[offset++] = successPatterns.length;
    for (const pat of successPatterns) {
      if (!pat) continue;
      const bytes = stringToBytes(pat);
      payload[offset++] = Math.min(bytes.length, 31);
      for (let i = 0; i < bytes.length && i < 31; i++) {
        payload[offset++] = bytes[i];
      }
    }

    // Failure patterns
    payload[offset++] = failurePatterns.length;
    for (const pat of failurePatterns) {
      if (!pat) continue;
      const bytes = stringToBytes(pat);
      payload[offset++] = Math.min(bytes.length, 31);
      for (let i = 0; i < bytes.length && i < 31; i++) {
        payload[offset++] = bytes[i];
      }
    }

    await sendCommand(CMD_SET_PATTERNS, payload.slice(0, offset));
    Alert.alert('Patterns Updated', `${successPatterns.length} success, ${failurePatterns.length} failure patterns`);
  };

  const updateSuccessPattern = (index, value) => {
    setSuccessPatterns(prev => prev.map((p, i) => i === index ? value : p));
  };

  const updateFailurePattern = (index, value) => {
    setFailurePatterns(prev => prev.map((p, i) => i === index ? value : p));
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Trigger Configuration</Text>

        <Text style={styles.subLabel}>Trigger Source</Text>
        <View style={styles.sourceGrid}>
          {TRIGGER_SOURCES.map(src => (
            <TouchableOpacity
              key={src.value}
              style={[styles.sourceBtn, triggerSource === src.value ? styles.sourceBtnActive : null]}
              onPress={() => setTriggerSource(src.value)}
            >
              <Text style={[styles.sourceBtnText, triggerSource === src.value ? styles.sourceBtnTextActive : null]}>
                {src.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <View style={styles.edgeRow}>
          <Text style={styles.edgeLabel}>Trigger Edge:</Text>
          <TouchableOpacity
            style={[styles.edgeBtn, triggerEdge === 0 ? styles.edgeBtnActive : null]}
            onPress={() => setTriggerEdge(0)}>
            <Text style={triggerEdge === 0 ? styles.edgeBtnTextActive : null}>Rising</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.edgeBtn, triggerEdge === 1 ? styles.edgeBtnActive : null]}
            onPress={() => setTriggerEdge(1)}>
            <Text style={triggerEdge === 1 ? styles.edgeBtnTextActive : null}>Falling</Text>
          </TouchableOpacity>
        </View>

        {triggerSource === 1 && (
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>UART Trigger Word</Text>
            <TextInput
              style={styles.paramInput}
              value={uartWord}
              onChangeText={setUartWord}
              placeholder="e.g. BOOT>"
              autoCapitalize="none"
            />
          </View>
        )}

        {triggerSource === 2 && (
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>Power Threshold (mV)</Text>
            <TextInput
              style={styles.paramInput}
              value={powerThreshold}
              onChangeText={setPowerThreshold}
              keyboardType="numeric"
            />
          </View>
        )}

        <TouchableOpacity style={styles.applyBtn} onPress={handleApplyTrigger}>
          <Text style={styles.applyBtnText}>Apply Trigger</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Target UART</Text>
        <View style={styles.paramRow}>
          <Text style={styles.paramLabel}>Baud Rate</Text>
          <TextInput
            style={styles.paramInput}
            value={targetBaud}
            onChangeText={setTargetBaud}
            keyboardType="numeric"
          />
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Success Patterns</Text>
        <Text style={styles.hint}>Target responses matching these will be classified as SUCCESS</Text>
        {successPatterns.map((pat, i) => (
          <View key={i} style={styles.patternRow}>
            <TextInput
              style={styles.patternInput}
              value={pat}
              onChangeText={v => updateSuccessPattern(i, v)}
              autoCapitalize="none"
            />
          </View>
        ))}
        <TouchableOpacity
          style={styles.addPatternBtn}
          onPress={() => setSuccessPatterns(prev => [...prev, ''])}
        >
          <Icon name="plus" size={20} color="#e91e63" />
          <Text style={styles.addPatternText}>Add Pattern</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Failure Patterns</Text>
        <Text style={styles.hint}>Target responses matching these will be classified as FAILURE</Text>
        {failurePatterns.map((pat, i) => (
          <View key={i} style={styles.patternRow}>
            <TextInput
              style={styles.patternInput}
              value={pat}
              onChangeText={v => updateFailurePattern(i, v)}
              autoCapitalize="none"
            />
          </View>
        ))}
        <TouchableOpacity
          style={styles.addPatternBtn}
          onPress={() => setFailurePatterns(prev => [...prev, ''])}
        >
          <Icon name="plus" size={20} color="#e91e63" />
          <Text style={styles.addPatternText}>Add Pattern</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.applyBtn} onPress={handleApplyPatterns}>
          <Text style={styles.applyBtnText}>Apply Patterns</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>PlasmaReaper v1.0.0</Text>
        <Text style={styles.aboutText}>Multi-Vector Fault Injection Toolkit</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>License: MIT (app) / GPL-2.0 (firmware) / CERN-OHL-S v2 (hardware)</Text>
        <Text style={styles.disclaimer}>
          ⚠️ For authorized security research only. Unauthorized fault injection
          may violate computer fraud, DMCA, and tampering laws. Always obtain
          written authorization before testing.
        </Text>
      </View>
    </ScrollView>
  );
}

function stringToBytes(str) {
  const bytes = [];
  for (let i = 0; i < str.length; i++) {
    bytes.push(str.charCodeAt(i) & 0xFF);
  }
  return bytes;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  section: { backgroundColor: '#fff', margin: 8, borderRadius: 8, padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  subLabel: { fontSize: 14, fontWeight: '600', color: '#555', marginBottom: 8 },
  sourceGrid: { flexDirection: 'row', flexWrap: 'wrap', marginBottom: 12 },
  sourceBtn: { paddingHorizontal: 12, paddingVertical: 8, borderWidth: 1, borderColor: '#ddd', borderRadius: 4, margin: 2 },
  sourceBtnActive: { backgroundColor: '#e91e63', borderColor: '#e91e63' },
  sourceBtnText: { fontSize: 12, color: '#333' },
  sourceBtnTextActive: { color: '#fff' },
  edgeRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  edgeLabel: { fontSize: 14, marginRight: 8 },
  edgeBtn: { paddingHorizontal: 12, paddingVertical: 6, borderWidth: 1, borderColor: '#ddd', borderRadius: 4, marginHorizontal: 4 },
  edgeBtnActive: { backgroundColor: '#e91e63', borderColor: '#e91e63' },
  edgeBtnTextActive: { color: '#fff' },
  paramRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  paramLabel: { flex: 1, fontSize: 14 },
  paramInput: { width: 140, borderWidth: 1, borderColor: '#ddd', borderRadius: 4, padding: 8, fontSize: 14 },
  applyBtn: { backgroundColor: '#e91e63', padding: 12, borderRadius: 6, alignItems: 'center', marginTop: 8 },
  applyBtnText: { color: '#fff', fontWeight: 'bold' },
  hint: { fontSize: 12, color: '#888', marginBottom: 8 },
  patternRow: { flexDirection: 'row', marginBottom: 6 },
  patternInput: { flex: 1, borderWidth: 1, borderColor: '#ddd', borderRadius: 4, padding: 8, fontSize: 13, fontFamily: 'monospace' },
  addPatternBtn: { flexDirection: 'row', alignItems: 'center', padding: 8 },
  addPatternText: { color: '#e91e63', marginLeft: 4 },
  aboutText: { fontSize: 13, color: '#555', marginBottom: 2 },
  disclaimer: { fontSize: 11, color: '#f44336', marginTop: 8, fontStyle: 'italic' },
  footer: { textAlign: 'center', color: '#999', fontSize: 12, padding: 16 },
});