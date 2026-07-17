// src/screens/CapabilityScreen.tsx — compose + send Source_Capabilities / Request.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { useState } from 'react';
import { View, Text, TextInput, Pressable, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../state/DeviceContext';
import { ConnectionBar } from '../components/ConnectionBar';
import { PD_DATA, buildRdo } from '../protocol';

// Standard USB-C PDO voltage presets (mV)
const VOLT_PRESETS = [5000, 9000, 15000, 20000, 28000, 36000, 48000];

export default function CapabilityScreen() {
  const { send } = useDevice();
  const [mv, setMv] = useState('20000');
  const [ma, setMa] = useState('3000');
  const [reqIdx, setReqIdx] = useState('0');
  const [reqMa, setReqMa] = useState('3000');
  const [injectType, setInjectType] = useState('0');

  return (
    <ScrollView style={styles.container}>
      <ConnectionBar />

      <Text style={styles.h1}>Spoof Source_Capabilities</Text>
      <Text style={styles.help}>
        Present a fake Source_Capabilities PDO to the sink. The device will then
        actually drive VBUS to the spoofed voltage through the LM5180 + eFuse path.
      </Text>

      <Text style={styles.label}>Voltage (mV)</Text>
      <View style={styles.presetRow}>
        {VOLT_PRESETS.map((v) => (
          <Pressable key={v} style={styles.preset} onPress={() => setMv(String(v))}>
            <Text style={styles.presetText}>{v / 1000}V</Text>
          </Pressable>
        ))}
      </View>
      <TextInput style={styles.input} value={mv} onChangeText={setMv} keyboardType="numeric" />

      <Text style={styles.label}>Current (mA)</Text>
      <TextInput style={styles.input} value={ma} onChangeText={setMa} keyboardType="numeric" />

      <Pressable
        style={styles.btn}
        onPress={() => {
          send({ cmd: 'mode', mode: 'spoof' });
          send({ cmd: 'spoof', mv: parseInt(mv) || 5000, ma: parseInt(ma) || 500 });
        }}
      >
        <Text style={styles.btnText}>Arm Spoof</Text>
      </Pressable>

      <View style={{ height: 1, backgroundColor: '#333', marginVertical: 16 }} />

      <Text style={styles.h1}>Send Request (RDO)</Text>
      <Text style={styles.help}>Inject a Request for PDO index + current toward the source.</Text>
      <Text style={styles.label}>PDO index</Text>
      <TextInput style={styles.input} value={reqIdx} onChangeText={setReqIdx} keyboardType="numeric" />
      <Text style={styles.label}>Operating current (mA)</Text>
      <TextInput style={styles.input} value={reqMa} onChangeText={setReqMa} keyboardType="numeric" />
      <Pressable
        style={styles.btn}
        onPress={() => {
          const idx = parseInt(reqIdx) || 0;
          const cur = parseInt(reqMa) || 500;
          const rdo = buildRdo(idx, cur);
          send({ cmd: 'inject', type: PD_DATA.REQUEST, nobj: 1, obj: [rdo] });
        }}
      >
        <Text style={styles.btnText}>Inject Request</Text>
      </Pressable>

      <View style={{ height: 1, backgroundColor: '#333', marginVertical: 16 }} />

      <Text style={styles.h1}>Inject Raw PD Message</Text>
      <Text style={styles.label}>Message type (0-15)</Text>
      <TextInput style={styles.input} value={injectType} onChangeText={setInjectType} keyboardType="numeric" />
      <Pressable
        style={styles.btn}
        onPress={() => send({ cmd: 'inject', type: parseInt(injectType) || 0, nobj: 0 })}
      >
        <Text style={styles.btnText}>Inject Control Message</Text>
      </Pressable>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d0d' },
  h1:       { color: '#eee', fontSize: 16, fontWeight: 'bold', padding: 12 },
  help:     { color: '#999', fontSize: 11, paddingHorizontal: 12, marginBottom: 8 },
  label:    { color: '#bbb', fontSize: 12, paddingHorizontal: 12, marginTop: 8 },
  input:    { backgroundColor: '#1a1a1a', color: '#eee', margin: 12, padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#333' },
  presetRow:{ flexDirection: 'row', flexWrap: 'wrap', paddingHorizontal: 12, gap: 6 },
  preset:   { backgroundColor: '#222', padding: 6, borderRadius: 6, marginRight: 6, marginBottom: 6 },
  presetText: { color: '#4cc9f0', fontSize: 11 },
  btn:      { backgroundColor: '#9b5de5', margin: 12, padding: 12, borderRadius: 8, alignItems: 'center' },
  btnText:  { color: 'white', fontWeight: 'bold' },
});