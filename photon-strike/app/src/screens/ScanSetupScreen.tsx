/**
 * ScanSetupScreen.tsx — build & submit a scan descriptor
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert, ScrollView } from 'react-native';
import { useBle, ScanDescriptor } from '../services/BleContext';

export default function ScanSetupScreen({ navigation }: any) {
  const ble = useBle();

  // Grid
  const [xStart, setXStart] = useState('400');
  const [xStop, setXStop] = useState('600');
  const [xStep, setXStep] = useState('5');
  const [yStart, setYStart] = useState('400');
  const [yStop, setYStop] = useState('600');
  const [yStep, setYStep] = useState('5');

  // Delay sweep (picoseconds)
  const [delayStart, setDelayStart] = useState('0');
  const [delayStop, setDelayStop] = useState('200000');   // 200 ns
  const [delayStep, setDelayStep] = useState('1000');     // 1 ns

  // Pulse width (ns)
  const [widthStart, setWidthStart] = useState('10');
  const [widthStop, setWidthStop] = useState('40');
  const [widthStep, setWidthStep] = useState('5');

  // Energy (DAC counts)
  const [energyStart, setEnergyStart] = useState('200');
  const [energyStop, setEnergyStop] = useState('800');
  const [energyStep, setEnergyStep] = useState('100');

  const [shotsPerPoint, setShotsPerPoint] = useState('3');
  const [dfaMode, setDfaMode] = useState('1');     // AES-128 Piret
  const [expectedHex, setExpectedHex] = useState('');   // hex of correct ciphertext

  const submit = async () => {
    const desc: ScanDescriptor = {
      magic: 0,
      trigSrc: 0, trigEdge: 0,
      trigPatternLo: 0, trigPatternHi: 0,
      trigMaskLo: 0, trigMaskHi: 0,
      powerThreshold: 0,
      xStart: +xStart, xStop: +xStop, xStep: +xStep,
      yStart: +yStart, yStop: +yStop, yStep: +yStep,
      delayStartPs: +delayStart, delayStopPs: +delayStop, delayStepPs: +delayStep,
      widthStartNs: +widthStart, widthStopNs: +widthStop, widthStepNs: +widthStep,
      energyStart: +energyStart, energyStop: +energyStop, energyStep: +energyStep,
      shotsPerPoint: +shotsPerPoint,
      expectedHash: 0,
      expectedLen: expectedHex.length / 2,
      dfaMode: +dfaMode,
    };
    await ble.sendScanDescriptor(desc);
    if (expectedHex) {
      const bytes = hexToBytes(expectedHex);
      await ble.sendExpectedOutput(bytes);
    }
    Alert.alert('Scan descriptor sent', 'Go to Scan Progress to start.');
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.section}>Spatial Grid (µm)</Text>
      <View style={styles.row}>
        <NumInput label="X start" v={xStart} set={setXStart} />
        <NumInput label="X stop"  v={xStop}  set={setXStop} />
        <NumInput label="X step"  v={xStep}  set={setXStep} />
      </View>
      <View style={styles.row}>
        <NumInput label="Y start" v={yStart} set={setYStart} />
        <NumInput label="Y stop"  v={yStop}  set={setYStop} />
        <NumInput label="Y step"  v={yStep}  set={setYStep} />
      </View>

      <Text style={styles.section}>Delay Sweep (ps)</Text>
      <View style={styles.row}>
        <NumInput label="Start ps" v={delayStart} set={setDelayStart} />
        <NumInput label="Stop ps"  v={delayStop}  set={setDelayStop} />
        <NumInput label="Step ps"  v={delayStep}  set={setDelayStep} />
      </View>

      <Text style={styles.section}>Pulse Width (ns)</Text>
      <View style={styles.row}>
        <NumInput label="Start ns" v={widthStart} set={setWidthStart} />
        <NumInput label="Stop ns"  v={widthStop}  set={setWidthStop} />
        <NumInput label="Step ns"  v={widthStep}  set={setWidthStep} />
      </View>

      <Text style={styles.section}>Energy (DAC counts)</Text>
      <View style={styles.row}>
        <NumInput label="Start" v={energyStart} set={setEnergyStart} />
        <NumInput label="Stop"  v={energyStop}  set={setEnergyStop} />
        <NumInput label="Step"  v={energyStep}  set={setEnergyStep} />
      </View>

      <Text style={styles.section}>Other</Text>
      <View style={styles.row}>
        <NumInput label="Shots/point" v={shotsPerPoint} set={setShotsPerPoint} />
        <NumInput label="DFA mode (0/1)" v={dfaMode} set={setDfaMode} />
      </View>

      <Text style={styles.section}>Expected (correct) ciphertext (hex)</Text>
      <TextInput
        style={styles.hexInput}
        value={expectedHex}
        onChangeText={setExpectedHex}
        placeholder="e.g. 5269ecb0…"
        placeholderTextColor="#555"
        autoCapitalize="none"
      />

      <TouchableOpacity style={styles.button} onPress={submit}>
        <Text style={styles.buttonText}>Send Scan Descriptor</Text>
      </TouchableOpacity>
      <TouchableOpacity style={[styles.button, { borderColor: '#4ecca3' }]} onPress={() => navigation.navigate('Trigger')}>
        <Text style={[styles.buttonText, { color: '#4ecca3' }]}>Configure Trigger →</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

function hexToBytes(hex: string): number[] {
  const clean = hex.replace(/\s+/g, '');
  const out: number[] = [];
  for (let i = 0; i < clean.length; i += 2)
    out.push(parseInt(clean.substr(i, 2), 16));
  return out;
}

function NumInput({ label, v, set }: { label: string; v: string; set: (s: string) => void }) {
  return (
    <View style={styles.numBox}>
      <Text style={styles.numLabel}>{label}</Text>
      <TextInput style={styles.numInput} keyboardType="numeric" value={v} onChangeText={set} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  section: { color: '#e94560', fontSize: 14, fontWeight: '700', marginTop: 16, marginBottom: 6 },
  row: { flexDirection: 'row', justifyContent: 'space-between' },
  numBox: { flex: 1, marginHorizontal: 4 },
  numLabel: { color: '#a0a0c0', fontSize: 11 },
  numInput: {
    backgroundColor: '#16213e', color: '#fff', borderRadius: 6,
    padding: 8, marginTop: 2, borderWidth: 1, borderColor: '#333',
  },
  hexInput: {
    backgroundColor: '#16213e', color: '#fff', borderRadius: 6,
    padding: 10, borderWidth: 1, borderColor: '#333', fontFamily: 'monospace',
  },
  button: {
    backgroundColor: '#16213e', padding: 14, borderRadius: 8,
    alignItems: 'center', marginVertical: 6, borderWidth: 1, borderColor: '#e94560',
  },
  buttonText: { color: '#e94560', fontSize: 16, fontWeight: '600' },
});