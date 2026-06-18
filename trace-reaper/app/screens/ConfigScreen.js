/**
 * TRACE-REAPER — Config screen
 *
 * Lets the operator set the session configuration before arming: target
 * cipher, leakage model, number of traces, sample window, trigger source,
 * analog threshold, input source (shunt vs EM), gain, and decimation.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useContext, useState } from 'react';
import { View, Text, TextInput, StyleSheet, TouchableOpacity, Picker, Alert } from 'react-native';
import { ConnectionContext, CIPHER, LEAK, TRIG, INPUT, GAIN } from '../utils/protocol';

export default function ConfigScreen() {
  const conn = useContext(ConnectionContext);
  const [cipher, setCipher] = useState(CIPHER.AES128);
  const [model, setModel] = useState(LEAK.HW_SBOX_OUT);
  const [traces, setTraces] = useState('5000');
  const [window, setWindow] = useState('2000');
  const [threshold, setThreshold] = useState('0');
  const [trig, setTrig] = useState(TRIG.EXTERNAL);
  const [input, setInput] = useState(INPUT.SHUNT);
  const [gain, setGain] = useState(GAIN.G0DB);
  const [decimate, setDecimate] = useState('1');
  const [label, setLabel] = useState('default');

  const apply = async () => {
    const cfg = {
      cipher,
      model,
      targetTraces: parseInt(traces, 10) || 5000,
      windowSamples: parseInt(window, 10) || 2000,
      trigThreshold: parseInt(threshold, 10) || 0,
      trigSrc: trig,
      input,
      gain,
      decimate: parseInt(decimate, 10) || 1,
      ptRandom: true,
      knownPt: new Uint8Array(16),
      sessionId: cryptoRandomId(),
      label,
    };
    try {
      await conn.configure(cfg);
      Alert.alert('Configured', 'Session configuration sent. Tap Arm on the Dashboard.');
    } catch (e) {
      Alert.alert('Configure failed', String(e));
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Session Configuration</Text>

      <Label text="Target cipher" />
      <Picker selectedValue={cipher} onValueChange={setCipher} style={styles.picker}>
        <Picker.Item label="AES-128" value={CIPHER.AES128} />
        <Picker.Item label="AES-256" value={CIPHER.AES256} />
        <Picker.Item label="DES"      value={CIPHER.DES} />
        <Picker.Item label="PRESENT"  value={CIPHER.PRESENT} />
      </Picker>

      <Label text="Leakage model" />
      <Picker selectedValue={model} onValueChange={setModel} style={styles.picker}>
        <Picker.Item label="HW(S-box output)"      value={LEAK.HW_SBOX_OUT} />
        <Picker.Item label="HW(S-box input)"       value={LEAK.HW_SBOX_IN} />
        <Picker.Item label="HD(S-box in -> out)"   value={LEAK.HD_SBOX_OUT} />
        <Picker.Item label="HW(MixColumns output)" value={LEAK.HW_MIXCOL} />
        <Picker.Item label="HD(last round)"        value={LEAK.HW_LASTROUND} />
      </Picker>

      <Label text="Target traces" />
      <TextInput style={styles.input} value={traces} onChangeText={setTraces} keyboardType="numeric" />

      <Label text="Window samples (<=8192)" />
      <TextInput style={styles.input} value={window} onChangeText={setWindow} keyboardType="numeric" />

      <Label text="Trigger source" />
      <Picker selectedValue={trig} onValueChange={setTrig} style={styles.picker}>
        <Picker.Item label="External BNC"        value={TRIG.EXTERNAL} />
        <Picker.Item label="Analog level"        value={TRIG.ANALOG} />
        <Picker.Item label="Template cross-corr" value={TRIG.TEMPLATE} />
        <Picker.Item label="Triggerless rolling" value={TRIG.NONE} />
      </Picker>

      <Label text="Analog trigger threshold (12-bit signed)" />
      <TextInput style={styles.input} value={threshold} onChangeText={setThreshold} keyboardType="numeric" />

      <Label text="Input source" />
      <Picker selectedValue={input} onValueChange={setInput} style={styles.picker}>
        <Picker.Item label="Shunt resistor (current)" value={INPUT.SHUNT} />
        <Picker.Item label="EM near-field probe"     value={INPUT.EM} />
      </Picker>

      <Label text="AFE gain" />
      <Picker selectedValue={gain} onValueChange={setGain} style={styles.picker}>
        <Picker.Item label="0 dB"  value={GAIN.G0DB} />
        <Picker.Item label="14 dB" value={GAIN.G14DB} />
        <Picker.Item label="28 dB" value={GAIN.G28DB} />
      </Picker>

      <Label text="Decimate (1..8, 0=none)" />
      <TextInput style={styles.input} value={decimate} onChangeText={setDecimate} keyboardType="numeric" />

      <Label text="Session label" />
      <TextInput style={styles.input} value={label} onChangeText={setLabel} />

      <TouchableOpacity style={styles.btn} onPress={apply}>
        <Text style={styles.btnText}>Apply configuration</Text>
      </TouchableOpacity>
    </View>
  );
}

function Label({ text }) {
  return <Text style={styles.label}>{text}</Text>;
}

function cryptoRandomId() {
  const a = new Uint8Array(16);
  for (let i = 0; i < 16; i++) a[i] = Math.floor(Math.random() * 256);
  return a;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1622', padding: 16 },
  title: { color: '#00e5ff', fontSize: 20, fontWeight: '700', marginTop: 16, marginBottom: 8 },
  label: { color: '#9fb1c7', fontSize: 12, marginTop: 12 },
  input: { backgroundColor: '#13243a', color: '#e6edf3', borderRadius: 6, padding: 10, marginTop: 4, fontSize: 13 },
  picker: { backgroundColor: '#13243a', color: '#e6edf3', marginTop: 4, borderRadius: 6 },
  btn: { backgroundColor: '#1f6feb', padding: 14, borderRadius: 8, marginTop: 24, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: '700', fontSize: 14 },
});