/**
 * SettingsScreen.js — Device Configuration
 * Author: jayis1
 */

import React, { useState } from 'react';
import {
  View, Text, ScrollView, TouchableOpacity, TextInput, Switch, StyleSheet, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { getConfig, updateConfig, setMode } from '../services/api';

const SettingsScreen = () => {
  const [wifiSSID, setWifiSSID] = useState('SATA-Phantom');
  const [wifiPass, setWifiPass] = useState('redteam42');
  const [wifiChannel, setWifiChannel] = useState('6');
  const [hostname, setHostname] = useState('sata-phantom');
  const [transport, setTransport] = useState(0);
  const [encryption, setEncryption] = useState(true);
  const [macRotation, setMacRotation] = useState(true);

  const handleSave = async () => {
    try {
      const config = { wifiSSID, wifiPass, wifiChannel, hostname, transport, encryption, macRotation };
      await updateConfig(config);
      Alert.alert('Saved', 'Configuration updated successfully');
    } catch (e) {
      Alert.alert('Error', 'Failed to save: ' + e.message);
    }
  };

  const handleModeSelect = (mode) => {
    setMode(mode).then(() => {
      Alert.alert('Mode Changed', `Device switched to mode ${mode}`);
    }).catch(e => {
      Alert.alert('Error', e.message);
    });
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionHeader}>WiFi Configuration</Text>
      <View style={styles.settingRow}>
        <Text style={styles.label}>AP SSID</Text>
        <TextInput style={styles.input} value={wifiSSID} onChangeText={setWifiSSID} />
      </View>
      <View style={styles.settingRow}>
        <Text style={styles.label}>AP Password</Text>
        <TextInput style={styles.input} value={wifiPass} onChangeText={setWifiPass} secureTextEntry />
      </View>
      <View style={styles.settingRow}>
        <Text style={styles.label}>Channel</Text>
        <TextInput style={[styles.input, { width: 60 }]} value={wifiChannel} onChangeText={setWifiChannel} keyboardType="numeric" />
      </View>

      <Text style={styles.sectionHeader}>Network</Text>
      <View style={styles.settingRow}>
        <Text style={styles.label}>mDNS Hostname</Text>
        <TextInput style={styles.input} value={hostname} onChangeText={setHostname} />
      </View>

      <Text style={styles.sectionHeader}>Exfiltration</Text>
      <View style={styles.settingRow}>
        <Text style={styles.label}>Transport</Text>
        <View style={styles.pickerRow}>
          {[
            { value: 0, label: 'TCP/TLS' },
            { value: 1, label: 'UDP/XOR' },
            { value: 2, label: 'BLE' },
          ].map((t) => (
            <TouchableOpacity
              key={t.value}
              style={[styles.pickerBtn, transport === t.value && styles.pickerBtnActive]}
              onPress={() => setTransport(t.value)}
            >
              <Text style={[styles.pickerText, transport === t.value && styles.pickerTextActive]}>{t.label}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>
      <View style={styles.settingRow}>
        <Text style={styles.label}>Encryption</Text>
        <Switch value={encryption} onValueChange={setEncryption} trackColor={{ false: '#333', true: '#00ff8844' }} thumbColor={encryption ? '#00ff88' : '#666'} />
      </View>
      <View style={styles.settingRow}>
        <Text style={styles.label}>MAC Rotation</Text>
        <Switch value={macRotation} onValueChange={setMacRotation} trackColor={{ false: '#333', true: '#00ff8844' }} thumbColor={macRotation ? '#00ff88' : '#666'} />
      </View>

      <Text style={styles.sectionHeader}>Operation Mode</Text>
      <View style={styles.modeGrid}>
        {[
          { mode: 0, name: 'Transparent', color: '#4a4' },
          { mode: 1, name: 'Monitor', color: '#48f' },
          { mode: 2, name: 'Active', color: '#f44' },
          { mode: 3, name: 'Exfil', color: '#fa4' },
          { mode: 4, name: 'Sleep', color: '#888' },
          { mode: 5, name: 'USB Config', color: '#f0f' },
        ].map((m) => (
          <TouchableOpacity
            key={m.mode}
            style={[styles.modeBtn, { borderColor: m.color }]}
            onPress={() => handleModeSelect(m.mode)}
          >
            <Text style={[styles.modeBtnText, { color: m.color }]}>{m.name}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <TouchableOpacity style={styles.saveBtn} onPress={handleSave}>
        <Icon name="content-save" size={20} color="#fff" />
        <Text style={styles.saveBtnText}>Save Configuration</Text>
      </TouchableOpacity>

      <View style={styles.footer}>
        <Text style={styles.footerText}>SATA Phantom v1.0.0</Text>
        <Text style={styles.footerSubtext}>Author: jayis1 © 2025</Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a', padding: 12 },
  sectionHeader: {
    color: '#00ff88', fontSize: 14, fontWeight: 'bold',
    marginTop: 20, marginBottom: 10, borderBottomWidth: 1, borderBottomColor: '#2a2a4a', paddingBottom: 4,
  },
  settingRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 10, gap: 8 },
  label: { color: '#aaa', fontSize: 13, width: 110 },
  input: {
    flex: 1, backgroundColor: '#12122a', borderRadius: 6, padding: 8,
    color: '#fff', fontSize: 14, borderWidth: 1, borderColor: '#2a2a4a',
  },
  pickerRow: { flexDirection: 'row', flex: 1, gap: 4 },
  pickerBtn: {
    flex: 1, padding: 8, borderRadius: 6, backgroundColor: '#12122a',
    alignItems: 'center', borderWidth: 1, borderColor: '#2a2a4a',
  },
  pickerBtnActive: { borderColor: '#00ff88' },
  pickerText: { color: '#888', fontSize: 11 },
  pickerTextActive: { color: '#00ff88', fontWeight: 'bold' },
  modeGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8, marginBottom: 16 },
  modeBtn: {
    width: '30%', padding: 12, borderRadius: 8, borderWidth: 1,
    backgroundColor: '#12122a', alignItems: 'center',
  },
  modeBtnText: { fontSize: 13, fontWeight: 'bold' },
  saveBtn: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'center',
    backgroundColor: '#00aa55', borderRadius: 10, padding: 14, marginTop: 8,
  },
  saveBtnText: { color: '#fff', fontWeight: 'bold', marginLeft: 8, fontSize: 15 },
  footer: { alignItems: 'center', marginTop: 24, marginBottom: 40 },
  footerText: { color: '#555', fontSize: 12 },
  footerSubtext: { color: '#444', fontSize: 10, marginTop: 2 },
});

export default SettingsScreen;
