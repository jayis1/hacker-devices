/**
 * ManualScreen.js — Manual glitch parameter editor + single shot
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: MIT
 *
 * Allows the operator to manually set all glitch parameters and fire
 * a single shot. Displays the result, including the captured VCC
 * waveform.
 */

import React, { useState, useContext } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet,
  ScrollView, Switch, ActivityIndicator, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { DeviceContext } from '../utils/deviceContext';
import { CMD_FIRE, CMD_GET_WAVEFORM, serializeParams, OUTCOME_LABELS, OUTCOME_COLORS } from '../utils/protocol';

const VECTORS = [
  { key: 'power', label: 'Power', mask: 0x01 },
  { key: 'clock', label: 'Clock', mask: 0x02 },
  { key: 'em', label: 'EM', mask: 0x04 },
];

export default function ManualScreen() {
  const { connected, sendCommand } = useContext(DeviceContext);

  const [params, setParams] = useState({
    vectorMask: 0x01,
    triggerOffsetNs: 1000,
    glitchWidthNs: 200,
    powerDepthMv: 900,
    powerSeriesR: 3,
    clockShape: 0,
    clockCycleOffset: 0,
    emPulseMv: 24000,
    emPulseWidthNs: 100,
    interVectorNs: 0,
    repeatCount: 1,
    repeatDelayNs: 100000,
  });

  const [firing, setFiring] = useState(false);
  const [result, setResult] = useState(null);
  const [waveform, setWaveform] = useState(null);

  const updateParam = (key, value) => {
    setParams(prev => ({ ...prev, [key]: value }));
  };

  const toggleVector = (mask) => {
    setParams(prev => ({ ...prev, vectorMask: prev.vectorMask ^ mask }));
  };

  const handleFire = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to PlasmaReaper first.');
      return;
    }

    setFiring(true);
    setResult(null);
    setWaveform(null);

    try {
      const payload = serializeParams(params);
      const resp = await sendCommand(CMD_FIRE, payload);

      if (resp && resp.type === 0x82) {
        setResult(resp.data);

        // Fetch waveform
        const wfResp = await sendCommand(CMD_GET_WAVEFORM);
        if (wfResp && wfResp.type === 0x86) {
          setWaveform(wfResp.data);
        }
      } else {
        Alert.alert('Fire Failed', 'No valid response from device.');
      }
    } catch (err) {
      Alert.alert('Error', err.message);
    }

    setFiring(false);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Glitch Vectors</Text>
        <View style={styles.vectorRow}>
          {VECTORS.map(v => (
            <TouchableOpacity
              key={v.key}
              style={[styles.vectorBtn,
                (params.vectorMask & v.mask) ? styles.vectorBtnActive : null]}
              onPress={() => toggleVector(v.mask)}
            >
              <Text style={[styles.vectorBtnText,
                (params.vectorMask & v.mask) ? styles.vectorBtnTextActive : null]}>
                {v.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Timing</Text>
        <ParamInput label="Trigger Offset (ns)" value={String(params.triggerOffsetNs)}
          onChangeText={v => updateParam('triggerOffsetNs', parseInt(v) || 0)} />
        <ParamInput label="Glitch Width (ns)" value={String(params.glitchWidthNs)}
          onChangeText={v => updateParam('glitchWidthNs', parseInt(v) || 0)} />
        <ParamInput label="Repeat Count" value={String(params.repeatCount)}
          onChangeText={v => updateParam('repeatCount', parseInt(v) || 1)} />
        <ParamInput label="Repeat Delay (ns)" value={String(params.repeatDelayNs)}
          onChangeText={v => updateParam('repeatDelayNs', parseInt(v) || 0)} />
      </View>

      {params.vectorMask & 0x01 ? (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Power Glitch</Text>
          <ParamInput label="Depth (mV)" value={String(params.powerDepthMv)}
            onChangeText={v => updateParam('powerDepthMv', parseInt(v) || 0)} />
          <ParamInput label="Series R Index (0-7)" value={String(params.powerSeriesR)}
            onChangeText={v => updateParam('powerSeriesR', Math.min(7, Math.max(0, parseInt(v) || 0)))} />
          <Text style={styles.resistanceHint}>
            R = {['0.5Ω','1Ω','2Ω','5Ω','10Ω','20Ω','33Ω','50Ω'][params.powerSeriesR] || '?'}
          </Text>
        </View>
      ) : null}

      {params.vectorMask & 0x02 ? (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Clock Glitch</Text>
          <View style={styles.shapeRow}>
            <TouchableOpacity
              style={[styles.shapeBtn, params.clockShape === 0 ? styles.shapeBtnActive : null]}
              onPress={() => updateParam('clockShape', 0)}>
              <Text style={styles.shapeBtnText}>Extra Edge</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.shapeBtn, params.clockShape === 1 ? styles.shapeBtnActive : null]}
              onPress={() => updateParam('clockShape', 1)}>
              <Text style={styles.shapeBtnText}>Suppress</Text>
            </TouchableOpacity>
          </View>
          <ParamInput label="Clock Cycle Offset" value={String(params.clockCycleOffset)}
            onChangeText={v => updateParam('clockCycleOffset', parseInt(v) || 0)} />
        </View>
      ) : null}

      {params.vectorMask & 0x04 ? (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>EM Pulse</Text>
          <ParamInput label="Pulse Voltage (mV)" value={String(params.emPulseMv)}
            onChangeText={v => updateParam('emPulseMv', parseInt(v) || 0)} />
          <ParamInput label="Pulse Width (ns)" value={String(params.emPulseWidthNs)}
            onChangeText={v => updateParam('emPulseWidthNs', parseInt(v) || 0)} />
        </View>
      ) : null}

      {params.vectorMask & 0x05 === 0x05 || (params.vectorMask & 0x03) === 0x03 ? (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Joint Injection</Text>
          <ParamInput label="Inter-Vector Delay (ns)" value={String(params.interVectorNs)}
            onChangeText={v => updateParam('interVectorNs', parseInt(v) || 0)} />
        </View>
      ) : null}

      <TouchableOpacity style={styles.fireBtn} onPress={handleFire} disabled={firing || !connected}>
        {firing ? <ActivityIndicator color="#fff" /> : (
          <View style={styles.fireBtnContent}>
            <Icon name="flash" size={20} color="#fff" />
            <Text style={styles.fireBtnText}>FIRE GLITCH</Text>
          </View>
        )}
      </TouchableOpacity>

      {result && (
        <View style={styles.resultCard}>
          <Text style={styles.resultTitle}>Result</Text>
          <Text style={[styles.resultOutcome, { color: OUTCOME_COLORS[result.outcome] || '#999' }]}>
            {OUTCOME_LABELS[result.outcome] || 'Unknown'}
          </Text>
          <Text style={styles.resultDetail}>Shots: {result.shotsFired}</Text>
          <Text style={styles.resultDetail}>Elapsed: {result.elapsedUs} µs</Text>
          {result.targetResponse ? (
            <View style={styles.responseBox}>
              <Text style={styles.responseLabel}>Target Response:</Text>
              <Text style={styles.responseText}>{result.targetResponse}</Text>
            </View>
          ) : null}
          {waveform && <WaveformView data={waveform.samples} />}
        </View>
      )}

      <Text style={styles.footer}>Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

function ParamInput({ label, value, onChangeText }) {
  return (
    <View style={styles.paramRow}>
      <Text style={styles.paramLabel}>{label}</Text>
      <TextInput
        style={styles.paramInput}
        value={value}
        onChangeText={onChangeText}
        keyboardType="numeric"
      />
    </View>
  );
}

function WaveformView({ data }) {
  if (!data || data.length === 0) return null;
  const maxVal = Math.max(...data);
  const minVal = Math.min(...data);
  const range = maxVal - minVal || 1;

  return (
    <View style={styles.waveformContainer}>
      <Text style={styles.waveformTitle}>VCC Waveform ({data.length} samples)</Text>
      <View style={styles.waveformBar}>
        {data.slice(0, 128).map((val, i) => {
          const height = ((val - minVal) / range) * 60;
          return (
            <View key={i} style={[styles.waveformBarItem, { height: Math.max(1, height) }]} />
          );
        })}
      </View>
      <Text style={styles.waveformScale}>Min: {(minVal * 3300 / 4096).toFixed(0)} mV  Max: {(maxVal * 3300 / 4096).toFixed(0)} mV</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  section: { backgroundColor: '#fff', margin: 8, borderRadius: 8, padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 12 },
  vectorRow: { flexDirection: 'row', justifyContent: 'space-around' },
  vectorBtn: { paddingHorizontal: 24, paddingVertical: 12, borderRadius: 6, borderWidth: 1, borderColor: '#ddd' },
  vectorBtnActive: { backgroundColor: '#e91e63', borderColor: '#e91e63' },
  vectorBtnText: { fontSize: 16, color: '#333' },
  vectorBtnTextActive: { color: '#fff', fontWeight: 'bold' },
  paramRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  paramLabel: { flex: 1, fontSize: 14, color: '#333' },
  paramInput: { width: 120, borderWidth: 1, borderColor: '#ddd', borderRadius: 4, padding: 8, fontSize: 14 },
  resistanceHint: { fontSize: 12, color: '#888', marginTop: 4 },
  shapeRow: { flexDirection: 'row', marginBottom: 12 },
  shapeBtn: { flex: 1, padding: 10, alignItems: 'center', borderWidth: 1, borderColor: '#ddd', marginHorizontal: 4, borderRadius: 4 },
  shapeBtnActive: { backgroundColor: '#e91e63', borderColor: '#e91e63' },
  shapeBtnText: { fontSize: 14 },
  fireBtn: { backgroundColor: '#e91e63', margin: 16, padding: 16, borderRadius: 8, alignItems: 'center' },
  fireBtnContent: { flexDirection: 'row', alignItems: 'center' },
  fireBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 18, marginLeft: 8 },
  resultCard: { backgroundColor: '#fff', margin: 16, borderRadius: 8, padding: 16 },
  resultTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  resultOutcome: { fontSize: 24, fontWeight: 'bold', marginBottom: 8 },
  resultDetail: { fontSize: 14, color: '#666' },
  responseBox: { marginTop: 12, padding: 8, backgroundColor: '#f0f0f0', borderRadius: 4 },
  responseLabel: { fontSize: 12, color: '#888', marginBottom: 4 },
  responseText: { fontSize: 14, fontFamily: 'monospace' },
  waveformContainer: { marginTop: 12 },
  waveformTitle: { fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  waveformBar: { flexDirection: 'row', height: 64, alignItems: 'flex-end', backgroundColor: '#1a1a2e', borderRadius: 4 },
  waveformBarItem: { flex: 1, backgroundColor: '#e91e63', marginHorizontal: 0.5 },
  waveformScale: { fontSize: 10, color: '#888', marginTop: 4 },
  footer: { textAlign: 'center', color: '#999', fontSize: 12, padding: 16 },
});