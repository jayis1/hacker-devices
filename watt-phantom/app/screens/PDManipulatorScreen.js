/**
 * PDManipulatorScreen.js — PD contract spoofing, role swaps, and attacks
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Provides controls for manipulating USB-C Power Delivery contracts:
 *  - Select PDO to advertise to the sink (target)
 *  - Select PDO to request from the source (charger)
 *  - Force VBUS to a specific voltage/current
 *  - Trigger Hard Reset, PR_Swap, DR_Swap
 *  - Contract oscillation (power DoS)
 *  - Overvoltage attack (attack mode required)
 *  - Advertise high-voltage attack PDOs
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert, TextInput } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const PDO_OPTIONS = [
  { label: '5V / 3A',    voltage: 5000,  current: 3000, index: 0 },
  { label: '9V / 3A',    voltage: 9000,  current: 3000, index: 1 },
  { label: '15V / 3A',   voltage: 15000, current: 3000, index: 2 },
  { label: '20V / 5A',   voltage: 20000, current: 5000, index: 3 },
];

export default function PDManipulatorScreen() {
  const { sendCommand, attackMode } = useDevice();
  const [selectedPdo, setSelectedPdo] = useState(0);
  const [customVoltage, setCustomVoltage] = useState('20000');
  const [customCurrent, setCustomCurrent] = useState('3000');
  const [oscillatePeriod, setOscillatePeriod] = useState('500');
  const [overvoltage, setOvervoltage] = useState('20000');
  const [log, setLog] = useState([]);

  const addLog = (msg) => {
    setLog((prev) => [...prev.slice(-10), `${new Date().toLocaleTimeString()} ${msg}`]);
  };

  const setSrcPdo = async (index) => {
    setSelectedPdo(index);
    const resp = await sendCommand(`SET SRC_PDO ${index}`, 2000);
    addLog(resp);
  };

  const setSnkPdo = async (index) => {
    const resp = await sendCommand(`SET SNK_PDO ${index}`, 2000);
    addLog(resp);
  };

  const forceVbus = async () => {
    const v = parseInt(customVoltage);
    const i = parseInt(customCurrent);
    if (v < 5000 || v > 48000) {
      Alert.alert('Invalid voltage', 'Voltage must be 5000-48000 mV');
      return;
    }
    const resp = await sendCommand(`SET VBUS ${v} ${i}`, 3000);
    addLog(resp);
  };

  const hardReset = async (port) => {
    const resp = await sendCommand(`HARD_RESET ${port}`, 2000);
    addLog(resp);
  };

  const prSwap = async () => {
    const resp = await sendCommand('PR_SWAP', 2000);
    addLog(resp);
  };

  const drSwap = async () => {
    const resp = await sendCommand('DR_SWAP', 2000);
    addLog(resp);
  };

  const startOscillate = async () => {
    const period = parseInt(oscillatePeriod);
    if (period < 100) {
      Alert.alert('Invalid period', 'Minimum 100ms to avoid hardware damage');
      return;
    }
    const resp = await sendCommand(`CONTRACT_OSCILLATE ${period}`, 2000);
    addLog(resp);
  };

  const advertiseAttackPdo = async () => {
    if (!attackMode) {
      Alert.alert('Attack mode required', 'Enable attack mode from the dashboard first.');
      return;
    }
    const resp = await sendCommand('ADVERTISE_ATTACK_PDO', 3000);
    addLog(resp);
  };

  const overvoltageAttack = async () => {
    if (!attackMode) {
      Alert.alert('Attack mode required', 'Enable attack mode from the dashboard first.');
      return;
    }
    const mv = parseInt(overvoltage);
    if (mv < 6000 || mv > 48000) {
      Alert.alert('Invalid voltage', 'Overvoltage range: 6000-48000 mV');
      return;
    }
    Alert.alert(
      'Confirm Overvoltage Attack',
      `This will force VBUS to ${mv / 1000}V on the target. This can permanently damage the target device. Continue?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Attack',
          style: 'destructive',
          onPress: async () => {
            const resp = await sendCommand(`OVERVOLTAGE_ATTACK ${mv}`, 5000);
            addLog(resp);
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>PD Manipulator</Text>
      <Text style={styles.subtitle}>USB-C Power Delivery contract control</Text>

      {/* Source PDO selection */}
      <Text style={styles.sectionTitle}>Source PDO (advertise to target)</Text>
      <View style={styles.pdoGrid}>
        {PDO_OPTIONS.map((pdo, i) => (
          <TouchableOpacity
            key={i}
            style={[styles.pdoButton, selectedPdo === i && styles.pdoButtonActive]}
            onPress={() => setSrcPdo(i)}
          >
            <Text style={styles.pdoButtonText}>{pdo.label}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Sink PDO selection */}
      <Text style={styles.sectionTitle}>Sink PDO (request from charger)</Text>
      <View style={styles.pdoGrid}>
        {PDO_OPTIONS.map((pdo, i) => (
          <TouchableOpacity
            key={i}
            style={styles.pdoButton}
            onPress={() => setSnkPdo(i)}
          >
            <Text style={styles.pdoButtonText}>{pdo.label}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Custom VBUS */}
      <Text style={styles.sectionTitle}>Force VBUS</Text>
      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={customVoltage}
          onChangeText={setCustomVoltage}
          placeholder="Voltage (mV)"
          keyboardType="numeric"
        />
        <TextInput
          style={styles.input}
          value={customCurrent}
          onChangeText={setCustomCurrent}
          placeholder="Current (mA)"
          keyboardType="numeric"
        />
        <TouchableOpacity style={styles.actionButton} onPress={forceVbus}>
          <Text style={styles.actionButtonText}>Set</Text>
        </TouchableOpacity>
      </View>

      {/* PD protocol attacks */}
      <Text style={styles.sectionTitle}>Protocol Attacks</Text>
      <View style={styles.buttonGrid}>
        <TouchableOpacity style={styles.attackButton} onPress={() => hardReset('src')}>
          <Text style={styles.attackButtonText}>Hard Reset (Src)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.attackButton} onPress={() => hardReset('snk')}>
          <Text style={styles.attackButtonText}>Hard Reset (Snk)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.attackButton} onPress={prSwap}>
          <Text style={styles.attackButtonText}>PR Swap</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.attackButton} onPress={drSwap}>
          <Text style={styles.attackButtonText}>DR Swap</Text>
        </TouchableOpacity>
      </View>

      {/* Contract oscillation */}
      <Text style={styles.sectionTitle}>Contract Oscillation (Power DoS)</Text>
      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={oscillatePeriod}
          onChangeText={setOscillatePeriod}
          placeholder="Period (ms)"
          keyboardType="numeric"
        />
        <TouchableOpacity style={styles.actionButton} onPress={startOscillate}>
          <Text style={styles.actionButtonText}>Start</Text>
        </TouchableOpacity>
      </View>

      {/* Overvoltage attack */}
      <Text style={styles.sectionTitle}>Overvoltage Attack {attackMode ? '' : '(requires attack mode)'}</Text>
      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={overvoltage}
          onChangeText={setOvervoltage}
          placeholder="Voltage (mV)"
          keyboardType="numeric"
        />
        <TouchableOpacity
          style={[styles.actionButton, !attackMode && styles.disabledButton]}
          onPress={overvoltageAttack}
          disabled={!attackMode}
        >
          <Text style={styles.actionButtonText}>Attack</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity
        style={[styles.actionButton, !attackMode && styles.disabledButton, { width: '100%', marginTop: 8 }]}
        onPress={advertiseAttackPdo}
        disabled={!attackMode}
      >
        <Text style={styles.actionButtonText}>Advertise Attack PDOs (up to 48V)</Text>
      </TouchableOpacity>

      {/* Log */}
      <Text style={styles.sectionTitle}>Activity Log</Text>
      {log.map((line, i) => (
        <Text key={i} style={styles.logLine}>{line}</Text>
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#00d4aa' },
  subtitle: { fontSize: 12, color: '#8b949e', marginBottom: 16 },
  sectionTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  pdoGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  pdoButton: { backgroundColor: '#21262d', padding: 10, borderRadius: 6, width: '48%', alignItems: 'center' },
  pdoButtonActive: { backgroundColor: '#00d4aa' },
  pdoButtonText: { color: '#e6edf3', fontWeight: 'bold', fontSize: 13 },
  inputRow: { flexDirection: 'row', gap: 8, alignItems: 'center' },
  input: { flex: 1, backgroundColor: '#161b22', color: '#e6edf3', padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#30363d', fontSize: 13 },
  actionButton: { backgroundColor: '#1f6feb', padding: 10, borderRadius: 6, paddingHorizontal: 16 },
  actionButtonText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  disabledButton: { backgroundColor: '#21262d', opacity: 0.5 },
  buttonGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  attackButton: { backgroundColor: '#2d1517', padding: 10, borderRadius: 6, width: '48%', alignItems: 'center', borderColor: '#f85149', borderWidth: 1 },
  attackButtonText: { color: '#f85149', fontWeight: 'bold', fontSize: 12 },
  logLine: { color: '#6e7681', fontSize: 11, fontFamily: 'monospace' },
});