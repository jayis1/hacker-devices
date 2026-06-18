/*
 * screens/RFSettingsScreen.js — RF settings screen
 *
 * Configures RF frequency, TX power, and displays current RF chain status.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity, ScrollView,
  Alert, Slider,
} from 'react-native';
import { bleManager } from '../utils/ble';
import { cmdSetFreq, cmdSetPower, RF_FREQS } from '../utils/protocol';

const PRESET_FREQS = [
  { name: 'GPS L1',      mhz: RF_FREQS.GPS_L1 },
  { name: 'Galileo E1',  mhz: RF_FREQS.GALILEO_E1 },
  { name: 'GLONASS L1',  mhz: RF_FREQS.GLONASS_L1 },
  { name: 'BeiDou B1',   mhz: RF_FREQS.BEIDOU_B1 },
];

export default function RFSettingsScreen() {
  const [freqMhz, setFreqMhz] = useState(String(RF_FREQS.GPS_L1));
  const [powerDbm, setPowerDbm] = useState(10);

  const applyFreq = async () => {
    const mhz = parseFloat(freqMhz);
    if (isNaN(mhz) || mhz < 1400 || mhz > 1700) {
      Alert.alert('Invalid Frequency', 'Must be 1400-1700 MHz (L1 band)');
      return;
    }
    const hz = Math.round(mhz * 1e6);
    await bleManager.send(cmdSetFreq(hz));
    Alert.alert('Frequency Set', `${mhz.toFixed(3)} MHz`);
  };

  const applyPower = async () => {
    await bleManager.send(cmdSetPower(Math.round(powerDbm)));
    Alert.alert('Power Set', `${Math.round(powerDbm)} dBm`);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>RF Settings</Text>
      </View>

      {/* Frequency */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CARRIER FREQUENCY</Text>

        <Text style={styles.inputLabel}>Frequency (MHz)</Text>
        <TextInput
          style={styles.input}
          keyboardType="numeric"
          value={freqMhz}
          onChangeText={setFreqMhz}
          placeholder="1575.420"
          placeholderTextColor="#555"
        />

        <TouchableOpacity style={styles.applyBtn} onPress={applyFreq}>
          <Text style={styles.applyBtnText}>Apply Frequency</Text>
        </TouchableOpacity>

        <Text style={styles.subLabel}>Quick Select:</Text>
        <View style={styles.freqPresets}>
          {PRESET_FREQS.map(f => (
            <TouchableOpacity
              key={f.name}
              style={styles.freqBtn}
              onPress={() => setFreqMhz(f.mhz.toFixed(3))}
            >
              <Text style={styles.freqBtnName}>{f.name}</Text>
              <Text style={styles.freqBtnVal}>{f.mhz.toFixed(3)} MHz</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* TX Power */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>TX POWER</Text>

        <Text style={styles.powerLabel}>
          {Math.round(powerDbm)} dBm
          {'  '}
          <Text style={styles.powerMw}>
            ({Math.pow(10, powerDbm / 10).toFixed(2)} mW)
          </Text>
        </Text>

        <Slider
          style={styles.slider}
          minimumValue={-30}
          maximumValue={20}
          step={1}
          value={powerDbm}
          onValueChange={setPowerDbm}
          minimumTrackTintColor="#22c55e"
          maximumTrackTintColor="#ef4444"
          thumbTintColor="#3b82f6"
        />

        <View style={styles.powerRange}>
          <Text style={styles.rangeLabel}>-30 dBm</Text>
          <Text style={styles.rangeLabel}>+20 dBm</Text>
        </View>

        <TouchableOpacity
          style={[styles.applyBtn, { backgroundColor: '#eab308' }]}
          onPress={applyPower}
        >
          <Text style={styles.applyBtnText}>Apply Power</Text>
        </TouchableOpacity>
      </View>

      {/* RF Chain Info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>RF CHAIN</Text>
        <View style={styles.chainRow}>
          <Text style={styles.chainLabel}>Transceiver:</Text>
          <Text style={styles.chainValue}>Si4463 ×2</Text>
        </View>
        <View style={styles.chainRow}>
          <Text style={styles.chainLabel}>LNA:</Text>
          <Text style={styles.chainValue}>SKY65404-21</Text>
        </View>
        <View style={styles.chainRow}>
          <Text style={styles.chainLabel}>RF Switch:</Text>
          <Text style={styles.chainValue}>HMC253</Text>
        </View>
        <View style={styles.chainRow}>
          <Text style={styles.chainLabel}>TCXO:</Text>
          <Text style={styles.chainValue}>26 MHz, 0.5 ppm</Text>
        </View>
        <View style={styles.chainRow}>
          <Text style={styles.chainLabel}>Antenna:</Text>
          <Text style={styles.chainValue}>SMA external</Text>
        </View>
      </View>

      {/* Warning */}
      <View style={styles.warning}>
        <Text style={styles.warningText}>
          ⚠ Transmitting GNSS signals without proper{'\n'}
          authorization is ILLEGAL in most jurisdictions.{'\n'}
          Always use RF-shielded environments.{'\n'}
          © jayis1 — Authorized Research Use Only
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#3b82f6', fontSize: 20, fontWeight: 'bold', fontFamily: 'monospace' },
  section: { margin: 10, padding: 15, backgroundColor: '#1a1a2e', borderRadius: 8 },
  sectionTitle: { color: '#3b82f6', fontSize: 11, fontWeight: 'bold', marginBottom: 10, letterSpacing: 1 },
  inputLabel: { color: '#888', fontSize: 12, marginTop: 10, marginBottom: 5 },
  input: { backgroundColor: '#2a2a3e', color: '#fff', borderRadius: 6, padding: 10, fontSize: 14, fontFamily: 'monospace' },
  applyBtn: { marginTop: 15, padding: 12, backgroundColor: '#22c55e', borderRadius: 8, alignItems: 'center' },
  applyBtnText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  subLabel: { color: '#888', fontSize: 11, marginTop: 15, marginBottom: 8 },
  freqPresets: { flexDirection: 'row', flexWrap: 'wrap' },
  freqBtn: { width: '48%', margin: 4, padding: 10, backgroundColor: '#2a2a3e', borderRadius: 6 },
  freqBtnName: { color: '#e0e0e0', fontSize: 12 },
  freqBtnVal: { color: '#3b82f6', fontSize: 10, fontFamily: 'monospace' },
  powerLabel: { color: '#fff', fontSize: 24, textAlign: 'center', fontFamily: 'monospace', marginVertical: 10 },
  powerMw: { color: '#888', fontSize: 12 },
  slider: { width: '100%', height: 40, marginVertical: 10 },
  powerRange: { flexDirection: 'row', justifyContent: 'space-between' },
  rangeLabel: { color: '#666', fontSize: 10 },
  chainRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  chainLabel: { color: '#888', fontSize: 13 },
  chainValue: { color: '#fff', fontSize: 13, fontFamily: 'monospace' },
  warning: { margin: 10, padding: 15, backgroundColor: '#2a0a0a', borderRadius: 8, borderWidth: 1, borderColor: '#ef4444' },
  warningText: { color: '#fbbf24', fontSize: 10, textAlign: 'center', lineHeight: 16, fontFamily: 'monospace' },
});