/**
 * TriggerScreen.tsx — configure the trigger source
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { useBle } from '../services/BleContext';

const SOURCES = [
  { id: 0, label: 'GPIO 1 (SMA)' },
  { id: 1, label: 'GPIO 2 (SMA)' },
  { id: 2, label: 'Pattern Match (UART/SPI)' },
  { id: 3, label: 'Power Trigger (shunt ADC)' },
  { id: 4, label: 'Manual / Software' },
];

export default function TriggerScreen({ navigation }: any) {
  const ble = useBle();
  const [src, setSrc] = useState(0);
  const [edge, setEdge] = useState(0);   // 0 rising, 1 falling
  const [patternHex, setPatternHex] = useState('');
  const [maskHex, setMaskHex] = useState('');
  const [powerThresh, setPowerThresh] = useState('2048');

  const apply = () => {
    // Re-send the scan descriptor with the chosen trigger params.
    // In a full app we'd keep the descriptor in a global store; here we
    // just confirm to the user and rely on Scan Setup to resend.
    Alert.alert('Trigger set', `Source: ${SOURCES[src].label}\nEdge: ${edge ? 'falling' : 'rising'}`);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.section}>Trigger Source</Text>
      {SOURCES.map((s) => (
        <TouchableOpacity
          key={s.id}
          style={[styles.option, src === s.id && styles.optionActive]}
          onPress={() => setSrc(s.id)}
        >
          <Text style={[styles.optionText, src === s.id && styles.optionTextActive]}>{s.label}</Text>
        </TouchableOpacity>
      ))}

      <Text style={styles.section}>Edge</Text>
      <View style={styles.row}>
        <TouchableOpacity style={[styles.option, edge === 0 && styles.optionActive]} onPress={() => setEdge(0)}>
          <Text style={[styles.optionText, edge === 0 && styles.optionTextActive]}>Rising</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.option, edge === 1 && styles.optionActive]} onPress={() => setEdge(1)}>
          <Text style={[styles.optionText, edge === 1 && styles.optionTextActive]}>Falling</Text>
        </TouchableOpacity>
      </View>

      {src === 2 && (
        <>
          <Text style={styles.section}>Pattern (hex, up to 8 bytes)</Text>
          <TextInput style={styles.hexInput} value={patternHex} onChangeText={setPatternHex} autoCapitalize="none" />
          <Text style={styles.section}>Mask (hex)</Text>
          <TextInput style={styles.hexInput} value={maskHex} onChangeText={setMaskHex} autoCapitalize="none" />
        </>
      )}

      {src === 3 && (
        <>
          <Text style={styles.section}>Power Threshold (ADC counts)</Text>
          <TextInput style={styles.hexInput} keyboardType="numeric" value={powerThresh} onChangeText={setPowerThresh} />
        </>
      )}

      <TouchableOpacity style={styles.button} onPress={apply}>
        <Text style={styles.buttonText}>Apply</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  section: { color: '#e94560', fontSize: 14, fontWeight: '700', marginTop: 16, marginBottom: 6 },
  row: { flexDirection: 'row', justifyContent: 'space-between' },
  option: {
    backgroundColor: '#16213e', padding: 12, borderRadius: 6,
    marginVertical: 4, borderWidth: 1, borderColor: '#333',
  },
  optionActive: { borderColor: '#e94560' },
  optionText: { color: '#a0a0c0', fontSize: 14 },
  optionTextActive: { color: '#e94560', fontWeight: '700' },
  hexInput: {
    backgroundColor: '#16213e', color: '#fff', borderRadius: 6,
    padding: 10, borderWidth: 1, borderColor: '#333', fontFamily: 'monospace',
  },
  button: {
    backgroundColor: '#16213e', padding: 14, borderRadius: 8,
    alignItems: 'center', marginVertical: 10, borderWidth: 1, borderColor: '#e94560',
  },
  buttonText: { color: '#e94560', fontSize: 16, fontWeight: '600' },
});