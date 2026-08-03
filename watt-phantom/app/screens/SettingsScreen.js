/**
 * SettingsScreen.js — safety limits, CC routing, USB data, firmware info
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Configuration screen for:
 *  - Safety limits (OVP/OCP thresholds)
 *  - CC routing mode (passthrough, isolated, crossover, MCU control)
 *  - USB data passthrough switch
 *  - Device info and firmware version
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, TextInput, Alert, Switch } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const CC_MODES = [
  { label: 'Passthrough (MITM)', value: 0 },
  { label: 'Isolated', value: 1 },
  { label: 'Crossover', value: 2 },
  { label: 'MCU Control', value: 3 },
];

export default function SettingsScreen() {
  const { sendCommand, disconnect } = useDevice();
  const [ovp, setOvp] = useState('20000');
  const [ocp, setOcp] = useState('5000');
  const [ccMode, setCcMode] = useState(0);
  const [usbPassthrough, setUsbPassthrough] = useState(true);
  const [deviceInfo, setDeviceInfo] = useState('');

  const applySafety = async () => {
    const ovpVal = parseInt(ovp);
    const ocpVal = parseInt(ocp);
    if (ovpVal < 5000 || ovpVal > 48000) {
      Alert.alert('Invalid OVP', 'OVP range: 5000-48000 mV');
      return;
    }
    if (ocpVal < 500 || ocpVal > 5000) {
      Alert.alert('Invalid OCP', 'OCP range: 500-5000 mA');
      return;
    }
    const resp = await sendCommand(`SET SAFETY ${ovpVal} ${ocpVal}`, 2000);
    Alert.alert('Safety Limits', resp);
  };

  const setRouting = async (mode) => {
    setCcMode(mode);
    const resp = await sendCommand(`SET CC_MODE ${mode}`, 2000);
    if (!resp || !resp.startsWith('OK')) {
      Alert.alert('Error', resp || 'Command failed');
    }
  };

  const toggleUsbPassthrough = async (value) => {
    setUsbPassthrough(value);
    const resp = await sendCommand(`SET USB_DATA ${value ? 1 : 0}`, 2000);
    if (!resp || !resp.startsWith('OK')) {
      Alert.alert('Error', resp || 'Command failed');
    }
  };

  const getDeviceInfo = async () => {
    const resp = await sendCommand('STATUS', 3000);
    setDeviceInfo(resp || 'No response');
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <Text style={styles.subtitle}>Safety limits & configuration</Text>

      {/* Safety limits */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Safety Limits</Text>
        <Text style={styles.warning}>
          These limits protect connected devices. The eFuses will disconnect
          VBUS if these thresholds are exceeded (unless attack mode is active).
        </Text>
        <View style={styles.inputRow}>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>OVP (mV)</Text>
            <TextInput
              style={styles.input}
              value={ovp}
              onChangeText={setOvp}
              keyboardType="numeric"
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>OCP (mA)</Text>
            <TextInput
              style={styles.input}
              value={ocp}
              onChangeText={setOcp}
              keyboardType="numeric"
            />
          </View>
        </View>
        <TouchableOpacity style={styles.applyButton} onPress={applySafety}>
          <Text style={styles.applyButtonText}>Apply Safety Limits</Text>
        </TouchableOpacity>
      </View>

      {/* CC Routing */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CC Line Routing</Text>
        {CC_MODES.map((mode) => (
          <TouchableOpacity
            key={mode.value}
            style={[styles.radioOption, ccMode === mode.value && styles.radioOptionActive]}
            onPress={() => setRouting(mode.value)}
          >
            <View style={[styles.radioDot, ccMode === mode.value && styles.radioDotActive]} />
            <Text style={[styles.radioLabel, ccMode === mode.value && styles.radioLabelActive]}>
              {mode.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* USB Data Switch */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>USB Data Path</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>USB 2.0 Passthrough</Text>
          <Switch
            value={usbPassthrough}
            onValueChange={toggleUsbPassthrough}
            trackColor={{ false: '#21262d', true: '#00d4aa' }}
            thumbColor="#fff"
          />
        </View>
        <Text style={styles.switchHint}>
          When enabled, USB D+/D- passes through between source and sink.
          Disable for power-only connections (covert channel mode).
        </Text>
      </View>

      {/* Device info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Info</Text>
        <TouchableOpacity style={styles.applyButton} onPress={getDeviceInfo}>
          <Text style={styles.applyButtonText}>Query Status</Text>
        </TouchableOpacity>
        {deviceInfo ? (
          <View style={styles.infoBox}>
            <Text style={styles.infoText}>{deviceInfo}</Text>
          </View>
        ) : null}
        <Text style={styles.versionText}>WattPhantom v1.0</Text>
        <Text style={styles.authorText}>Author: jayis1</Text>
        <Text style={styles.licenseText}>Hardware: CERN-OHL-S v2 | Firmware: GPL-2.0 | App: MIT</Text>
      </View>

      {/* Disconnect */}
      <TouchableOpacity style={styles.disconnectButton} onPress={disconnect}>
        <Text style={styles.disconnectButtonText}>Disconnect</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#00d4aa' },
  subtitle: { fontSize: 12, color: '#8b949e', marginBottom: 16 },
  section: { backgroundColor: '#161b22', padding: 14, borderRadius: 8, marginBottom: 12, borderWidth: 1, borderColor: '#30363d' },
  sectionTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  warning: { color: '#f85149', fontSize: 11, marginBottom: 12 },
  inputRow: { flexDirection: 'row', gap: 12, marginBottom: 12 },
  inputGroup: { flex: 1 },
  inputLabel: { color: '#8b949e', fontSize: 11, marginBottom: 4 },
  input: { backgroundColor: '#0d1117', color: '#e6edf3', padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#30363d', fontSize: 13 },
  applyButton: { backgroundColor: '#1f6feb', padding: 12, borderRadius: 8, alignItems: 'center', marginTop: 4 },
  applyButtonText: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
  radioOption: { flexDirection: 'row', alignItems: 'center', padding: 10, borderRadius: 6, marginBottom: 4 },
  radioOptionActive: { backgroundColor: '#21262d' },
  radioDot: { width: 16, height: 16, borderRadius: 8, borderWidth: 2, borderColor: '#6e7681', marginRight: 10 },
  radioDotActive: { borderColor: '#00d4aa', backgroundColor: '#00d4aa' },
  radioLabel: { color: '#8b949e', fontSize: 13 },
  radioLabelActive: { color: '#e6edf3', fontWeight: 'bold' },
  switchRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 },
  switchLabel: { color: '#e6edf3', fontSize: 14 },
  switchHint: { color: '#6e7681', fontSize: 11 },
  infoBox: { backgroundColor: '#0d1117', padding: 10, borderRadius: 6, marginTop: 8, borderWidth: 1, borderColor: '#30363d' },
  infoText: { color: '#8b949e', fontSize: 11, fontFamily: 'monospace' },
  versionText: { color: '#00d4aa', fontSize: 14, fontWeight: 'bold', marginTop: 12, textAlign: 'center' },
  authorText: { color: '#8b949e', fontSize: 12, textAlign: 'center' },
  licenseText: { color: '#6e7681', fontSize: 10, textAlign: 'center', marginTop: 4 },
  disconnectButton: { backgroundColor: '#2d1517', padding: 14, borderRadius: 8, alignItems: 'center', marginTop: 8, marginBottom: 20, borderColor: '#f85149', borderWidth: 1 },
  disconnectButtonText: { color: '#f85149', fontWeight: 'bold', fontSize: 14 },
});