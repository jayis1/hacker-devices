/**
 * TeslaPhantom — EMFI Control Screen
 * Configure and fire electromagnetic fault injection pulses.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Slider, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function EmfiControlScreen() {
  const { connected, status, sendCommand } = useDevice();
  const [voltage, setVoltage] = useState(200);
  const [width, setWidth] = useState(50);
  const [delay, setDelay] = useState(1000);
  const [count, setCount] = useState(1);
  const [interval, setIntervalUs] = useState(0);
  const [triggerSrc, setTriggerSrc] = useState('manual');

  const handleFire = () => {
    Alert.alert(
      '⚠ EMFI Pulse',
      `Fire ${count} pulse(s) at ${voltage}V, ${width}ns width?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'FIRE',
          onPress: () => sendCommand({
            cmd: 'emfi_pulse',
            voltage: voltage,
            width_ns: width,
            delay_ns: delay,
            count: count,
            interval_us: interval
          })
        }
      ]
    );
  };

  const handleArm = () => {
    sendCommand({
      cmd: 'emfi_arm',
      voltage: voltage,
      width_ns: width,
      delay_ns: delay
    });
  };

  const handleDisarm = () => {
    sendCommand({ cmd: 'emfi_disarm' });
  };

  return (
    <View style={styles.container}>
      <Text style={styles.sectionTitle}>EMFI Pulse Parameters</Text>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Voltage: {voltage}V</Text>
        <Slider
          style={styles.slider}
          minimumValue={50}
          maximumValue={400}
          step={10}
          value={voltage}
          onValueChange={setVoltage}
          minimumTrackTintColor="#e74c3c"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Pulse Width: {width}ns</Text>
        <Slider
          style={styles.slider}
          minimumValue={5}
          maximumValue={200}
          step={5}
          value={width}
          onValueChange={setWidth}
          minimumTrackTintColor="#e74c3c"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Delay: {delay}ns</Text>
        <Slider
          style={styles.slider}
          minimumValue={0}
          maximumValue={10000}
          step={100}
          value={delay}
          onValueChange={setDelay}
          minimumTrackTintColor="#3498db"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Pulse Count: {count}</Text>
        <TextInput
          style={styles.input}
          value={String(count)}
          onChangeText={(v) => setCount(parseInt(v) || 1)}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Interval: {interval}µs</Text>
        <TextInput
          style={styles.input}
          value={String(interval)}
          onChangeText={(v) => setIntervalUs(parseInt(v) || 0)}
          keyboardType="numeric"
        />
      </View>

      <Text style={styles.sectionTitle}>Trigger Source</Text>
      <View style={styles.triggerRow}>
        {['manual', 'external', 'timer', 'magnetic'].map((src) => (
          <TouchableOpacity
            key={src}
            style={[styles.triggerBtn, triggerSrc === src && styles.triggerBtnActive]}
            onPress={() => {
              setTriggerSrc(src);
              sendCommand({ cmd: 'set_trigger', source: ['manual', 'external', 'timer', 'magnetic'].indexOf(src) });
            }}
          >
            <Text style={[styles.triggerBtnText, triggerSrc === src && styles.triggerBtnTextActive]}>
              {src.toUpperCase()}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.actionRow}>
        <TouchableOpacity style={[styles.button, styles.armBtn]} onPress={handleArm}>
          <Text style={styles.buttonText}>ARM (Ext Trig)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.button, styles.disarmBtn]} onPress={handleDisarm}>
          <Text style={styles.buttonText}>DISARM</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity
        style={[styles.button, styles.fireBtn, !connected && styles.disabledBtn]}
        onPress={handleFire}
        disabled={!connected}
      >
        <Text style={styles.fireText}>⚡ FIRE PULSE ⚡</Text>
      </TouchableOpacity>

      {status.armed && (
        <Text style={styles.armedWarning}>⚠ DEVICE ARMED — {status.hvVoltage}V CHARGED</Text>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#e74c3c', marginTop: 16, marginBottom: 8 },
  paramRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginBottom: 12 },
  label: { fontSize: 14, color: '#bbb', flex: 1 },
  slider: { flex: 1.5, height: 40 },
  input: { backgroundColor: '#1a1a2e', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, paddingHorizontal: 10, paddingVertical: 6, width: 80, textAlign: 'center' },
  triggerRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 16 },
  triggerBtn: { flex: 1, backgroundColor: '#1a1a2e', paddingVertical: 10, marginHorizontal: 2,
                borderRadius: 4, borderWidth: 1, borderColor: '#333' },
  triggerBtnActive: { backgroundColor: '#e74c3c', borderColor: '#e74c3c' },
  triggerBtnText: { color: '#95a5a6', fontSize: 11, textAlign: 'center', fontWeight: 'bold' },
  triggerBtnTextActive: { color: '#fff' },
  actionRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  button: { flex: 1, paddingVertical: 14, borderRadius: 6, marginHorizontal: 4, alignItems: 'center' },
  armBtn: { backgroundColor: '#e67e22' },
  disarmBtn: { backgroundColor: '#27ae60' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  fireBtn: { backgroundColor: '#c0392b', paddingVertical: 20, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  disabledBtn: { opacity: 0.4 },
  fireText: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  armedWarning: { color: '#e74c3c', fontSize: 14, textAlign: 'center', marginTop: 12, fontWeight: 'bold' },
});