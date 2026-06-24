/*
 * lora-phantom / app / screens / SettingsScreen.js
 * Region select, power limit, link key, SD format, firmware OTA.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Picker, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, REGIONS, REGION_NAMES, hexToBytes } from '../utils/protocol';

export default function SettingsScreen() {
  const { connected, status, sendCommand } = useDevice();
  const [region, setRegion] = useState(REGIONS.EU868);
  const [txPower, setTxPower] = useState('14');
  const [linkKey, setLinkKey] = useState('');
  const [otaProgress, setOtaProgress] = useState(null);

  const applyRegion = () => {
    sendCommand(CMD.SET_REGION, new Uint8Array([region]));
  };

  const applyTxPower = () => {
    const p = parseInt(txPower, 10);
    sendCommand(CMD.SET_TXPOWER, new Uint8Array([p & 0xFF]));
  };

  const applyLinkKey = () => {
    if (linkKey.length !== 32) {
      Alert.alert('Invalid Key', 'Link key must be 32 hex characters (128-bit).');
      return;
    }
    const keyBytes = hexToBytes(linkKey);
    sendCommand(CMD.SET_LINK_KEY, keyBytes);
    Alert.alert('Link Key Set', 'BLE backhaul encryption key updated.');
  };

  const formatSd = () => {
    Alert.alert('Format SD?', 'This will erase all captures on the SD card.',
      [{ text: 'Cancel' }, { text: 'Format', onPress: () => sendCommand(CMD.SD_FORMAT) }]);
  };

  const regionLimits = {
    [REGIONS.EU868]: '14 dBm / 1% duty',
    [REGIONS.US915]: '21 dBm / no duty',
    [REGIONS.AS923]: '16 dBm / 1% duty',
    [REGIONS.CN470]: '17 dBm / no duty',
    [REGIONS.AU915]: '30 dBm / no duty',
    [REGIONS.IN865]: '30 dBm / no duty',
    [REGIONS.KR920]: '14 dBm / no duty',
    [REGIONS.RU864]: '16 dBm / 1% duty',
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>⚙️ Settings</Text>

      {/* Region */}
      <Text style={styles.section}>LoRaWAN Region</Text>
      <View style={styles.pickerContainer}>
        <Picker
          selectedValue={region}
          onValueChange={setRegion}
          style={styles.picker}
          dropdownIconColor="#00ff88">
          {Object.entries(REGION_NAMES).map(([k, v]) => (
            <Picker.Item key={k} label={`${v} (${regionLimits[k]})`} value={parseInt(k)} color="#aaa" />
          ))}
        </Picker>
      </View>
      <TouchableOpacity style={styles.button} onPress={applyRegion} disabled={!connected}>
        <Text style={styles.btnText}>Apply Region</Text>
      </TouchableOpacity>

      {/* TX Power */}
      <Text style={styles.section}>TX Power Limit</Text>
      <View style={styles.row}>
        <TextInput style={styles.input} value={txPower} onChangeText={setTxPower}
          keyboardType="number-pad" />
        <Text style={styles.unit}>dBm</Text>
        <TouchableOpacity style={styles.smallBtn} onPress={applyTxPower} disabled={!connected}>
          <Text style={styles.smallBtnText}>Set</Text>
        </TouchableOpacity>
      </View>
      <Text style={styles.hint}>Firmware enforces regional maximum. Excess values are clamped.</Text>

      {/* BLE Link Key */}
      <Text style={styles.section}>BLE Backhaul Encryption Key</Text>
      <TextInput
        style={styles.keyInput}
        value={linkKey}
        onChangeText={setLinkKey}
        placeholder="32 hex chars (AES-128 key)"
        placeholderTextColor="#555"
        maxLength={32}
      />
      <TouchableOpacity style={styles.button} onPress={applyLinkKey} disabled={!connected || linkKey.length !== 32}>
        <Text style={styles.btnText}>Set Link Key</Text>
      </TouchableOpacity>

      {/* SD Card */}
      <Text style={styles.section}>SD Card</Text>
      {status && (
        <Text style={styles.sdInfo}>Free space: {Math.round(status.sdFreeKb / 1024)} MB</Text>
      )}
      <TouchableOpacity style={styles.dangerBtn} onPress={formatSd} disabled={!connected}>
        <Text style={styles.dangerBtnText}>Format SD Card</Text>
      </TouchableOpacity>

      {/* Firmware */}
      <Text style={styles.section}>Firmware</Text>
      <Text style={styles.fwInfo}>Version: 1.0.0 (jayis1)</Text>
      <Text style={styles.fwInfo}>Target: STM32H743VIT6</Text>
      <Text style={styles.fwInfo}>Radio: SX1262 + SKY66122</Text>
      <Text style={styles.fwInfo}>BLE: nRF52840</Text>

      {/* About */}
      <Text style={styles.section}>About</Text>
      <Text style={styles.aboutText}>LoRaPhantom v1.0</Text>
      <Text style={styles.aboutText}>LoRaWAN & LoRa Infiltration Device</Text>
      <Text style={styles.aboutText}>Author: jayis1</Text>
      <Text style={styles.aboutText}>License: GPL-2.0 (FW) / MIT (App) / CERN-OHL-S (HW)</Text>
      <Text style={styles.aboutText}>© 2026 jayis1. All rights reserved.</Text>

      <View style={styles.legalBox}>
        <Text style={styles.legalTitle}>⚠️ Legal Disclaimer</Text>
        <Text style={styles.legalText}>
          This device is for authorized security research and penetration
          testing ONLY. Unauthorized interception, key extraction, replay,
          injection, or transmission on LoRa/LoRaWAN networks may violate
          computer fraud (CFAA), wiretap, and radio regulations. Always
          obtain written authorization. The author assumes no liability for
          misuse.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 10 },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 18, marginBottom: 6 },
  pickerContainer: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#333',
                     borderRadius: 4, marginBottom: 8 },
  picker: { color: '#ccc' },
  button: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
            borderRadius: 6, padding: 12, alignItems: 'center' },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: '600' },
  row: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  input: { flex: 1, backgroundColor: '#1a1a1a', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 8, fontSize: 14 },
  unit: { color: '#888', fontSize: 13 },
  smallBtn: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
              borderRadius: 4, padding: 10, paddingHorizontal: 15 },
  smallBtnText: { color: '#00ff88', fontSize: 13 },
  hint: { color: '#666', fontSize: 11, marginTop: 4 },
  keyInput: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
              borderRadius: 4, padding: 10, fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  sdInfo: { color: '#aaa', fontSize: 13, marginBottom: 8 },
  dangerBtn: { backgroundColor: '#3a0a0a', borderWidth: 1, borderColor: '#ff6666',
               borderRadius: 6, padding: 12, alignItems: 'center' },
  dangerBtnText: { color: '#ff6666', fontSize: 14, fontWeight: '600' },
  fwInfo: { color: '#888', fontSize: 12, lineHeight: 18 },
  aboutText: { color: '#888', fontSize: 12, lineHeight: 16 },
  legalBox: { backgroundColor: '#2a1a0a', borderWidth: 1, borderColor: '#ff6600',
              borderRadius: 8, padding: 15, marginTop: 15, marginBottom: 30 },
  legalTitle: { color: '#ffaa44', fontSize: 14, fontWeight: 'bold', marginBottom: 5 },
  legalText: { color: '#ffaa44', fontSize: 11, lineHeight: 16 },
});