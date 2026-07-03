/**
 * ScenarioEditorScreen.js — visual scenario builder
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Lets the operator build a time-varying phantom scenario as a series of
 * steps (ramps of range/velocity/RCS with durations) and preview the
 * resulting range/velocity profile before committing to the device.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ScenarioEditorScreen() {
  const { commands, rangeCm, velMmps, rcsQdb } = useDevice();
  const [steps, setSteps] = useState([
    { id: 0, type: 'SET',  param: 'RANGE', value: 3000, dur: 0 },
    { id: 1, type: 'SET',  param: 'VEL',   value: 0,    dur: 0 },
    { id: 2, type: 'RAMP', param: 'RANGE', value: 1000, dur: 5000 },
  ]);
  const [selParam, setSelParam] = useState('RANGE');
  const [value, setValue] = useState('5000');
  const [dur, setDur] = useState('2000');
  const [nextId, setNextId] = useState(3);

  const addStep = (type) => {
    setSteps([...steps, {
      id: nextId,
      type,
      param: selParam,
      value: parseInt(value, 10) || 0,
      dur: type === 'RAMP' ? (parseInt(dur, 10) || 1000) : 0,
    }]);
    setNextId(nextId + 1);
  };

  const removeStep = (id) => setSteps(steps.filter(s => s.id !== id));

  const sendLive = async () => {
    // Execute steps sequentially (simplified: sends the final value of each
    // step; full streaming of ramps is done on-device via scenario VM).
    for (const s of steps) {
      if (s.param === 'RANGE') await commands.setRange(s.value);
      else if (s.param === 'VEL') await commands.setVelocity(s.value);
      else if (s.param === 'RCS') await commands.setRcs(s.value);
      await new Promise(r => setTimeout(r, Math.max(s.dur, 50)));
    }
  };

  const renderItem = ({ item }) => (
    <View style={styles.stepRow}>
      <Text style={styles.stepType}>{item.type}</Text>
      <Text style={styles.stepParam}>{item.param}</Text>
      <Text style={styles.stepValue}>{item.value}</Text>
      {item.type === 'RAMP' && <Text style={styles.stepDur}>{item.dur}ms</Text>}
      <TouchableOpacity onPress={() => removeStep(item.id)}>
        <Text style={styles.removeBtn}>✕</Text>
      </TouchableOpacity>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Scenario Builder</Text>

      <View style={styles.inputRow}>
        <TouchableOpacity
          style={[styles.paramBtn, selParam === 'RANGE' && styles.paramBtnSel]}
          onPress={() => setSelParam('RANGE')}>
          <Text style={styles.paramBtnText}>RANGE (cm)</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.paramBtn, selParam === 'VEL' && styles.paramBtnSel]}
          onPress={() => setSelParam('VEL')}>
          <Text style={styles.paramBtnText}>VEL (mm/s)</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.paramBtn, selParam === 'RCS' && styles.paramBtnSel]}
          onPress={() => setSelParam('RCS')}>
          <Text style={styles.paramBtnText}>RCS (qdB)</Text>
        </TouchableOpacity>
      </View>

      <TextInput
        style={styles.input}
        value={value}
        onChangeText={setValue}
        keyboardType="numeric"
        placeholder="value"
        placeholderTextColor="#555"
      />
      <TextInput
        style={styles.input}
        value={dur}
        onChangeText={setDur}
        keyboardType="numeric"
        placeholder="duration (ms, for RAMP)"
        placeholderTextColor="#555"
      />

      <View style={styles.inputRow}>
        <TouchableOpacity style={styles.addBtn} onPress={() => addStep('SET')}>
          <Text style={styles.addBtnText}>+ SET</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.addBtn} onPress={() => addStep('RAMP')}>
          <Text style={styles.addBtnText}>+ RAMP</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        data={steps}
        keyExtractor={item => String(item.id)}
        renderItem={renderItem}
        style={styles.list}
        ListEmptyComponent={<Text style={styles.empty}>No steps yet</Text>}
      />

      <TouchableOpacity style={styles.runBtn} onPress={sendLive}>
        <Text style={styles.runBtnText}>SEND & RUN</Text>
      </TouchableOpacity>

      <View style={styles.liveBox}>
        <Text style={styles.liveLabel}>LIVE</Text>
        <Text style={styles.liveValue}>
          {(rangeCm/100).toFixed(1)} m · {(velMmps/278).toFixed(0)} km/h · {(rcsQdb/8).toFixed(1)} dBsm
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 16 },
  title: { color: '#00ff9f', fontSize: 20, fontWeight: 'bold', marginBottom: 12 },
  inputRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  paramBtn: { flex: 1, backgroundColor: '#1a1a2e', padding: 10, borderRadius: 6, alignItems: 'center' },
  paramBtnSel: { backgroundColor: '#2a4e2a' },
  paramBtnText: { color: '#ccc', fontSize: 12 },
  input: { backgroundColor: '#1a1a2e', color: '#fff', padding: 12, borderRadius: 6, marginBottom: 8 },
  addBtn: { flex: 1, backgroundColor: '#2a2a4e', padding: 12, borderRadius: 6, alignItems: 'center', marginRight: 8 },
  addBtnText: { color: '#00ff9f', fontWeight: 'bold' },
  list: { flex: 1, marginTop: 8 },
  stepRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1a1a2e', padding: 10, borderRadius: 6, marginBottom: 4, gap: 8 },
  stepType: { color: '#00ff9f', fontWeight: 'bold', width: 50, fontSize: 12 },
  stepParam: { color: '#aaa', flex: 1, fontSize: 12 },
  stepValue: { color: '#fff', flex: 1, fontSize: 12 },
  stepDur: { color: '#888', fontSize: 11 },
  removeBtn: { color: '#ff5555', fontSize: 16, marginLeft: 8 },
  empty: { color: '#555', fontStyle: 'italic', marginTop: 20, textAlign: 'center' },
  runBtn: { backgroundColor: '#2a6e2a', padding: 16, borderRadius: 10, alignItems: 'center', marginTop: 12 },
  runBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  liveBox: { backgroundColor: '#1a1a2e', padding: 12, borderRadius: 6, marginTop: 12 },
  liveLabel: { color: '#666', fontSize: 11 },
  liveValue: { color: '#00ff9f', fontSize: 14, marginTop: 4 },
});