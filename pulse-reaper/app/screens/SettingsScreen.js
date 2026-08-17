/**
 * SettingsScreen.js — BLE key management, firmware update, device info
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, TextInput, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function SettingsScreen() {
  const { disconnect, sendCommand, firmwareVersion, status, battery } = useDevice();
  const [newKey, setNewKey] = useState('');

  const handlePing = async () => {
    const resp = await sendCommand(Buffer.from([0x01]));  // CMD.PING
    Alert.alert('Ping', resp || 'no response');
  };

  const handleStatus = async () => {
    const resp = await sendCommand(Buffer.from([0x02]));  // CMD.GET_STATUS
    Alert.alert('Device Status', resp || 'no response');
  };

  const handleFwUpdate = () => {
    Alert.alert(
      'Firmware Update',
      'Firmware update over BLE (signed image) is not yet implemented in this reference app. ' +
      'In a production build, a signed .bin image is transferred over BLE and the bootloader verifies it.',
      [{ text: 'OK' }]
    );
  };

  const handleRekey = () => {
    if (newKey.length < 8) {
      Alert.alert('Invalid key', 'Enter at least 8 characters');
      return;
    }
    Alert.alert(
      'Re-key BLE Session',
      'This would trigger a new ECDH P-256 handshake with the nRF52840 co-processor. ' +
      'The new session key is derived from the shared secret via HKDF-SHA256. ' +
      '(Reference app: not sent — implement in production.)',
      [{ text: 'OK' }]
    );
  };

  const handleDisconnect = async () => {
    await disconnect();
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.infoCard}>
        <Text style={styles.infoLabel}>Firmware:</Text>
        <Text style={styles.infoValue}>{firmwareVersion || 'unknown'}</Text>
        <Text style={styles.infoLabel}>Status:</Text>
        <Text style={styles.infoValue}>{status}</Text>
        <Text style={styles.infoLabel}>Battery:</Text>
        <Text style={styles.infoValue}>{battery}%</Text>
        <Text style={styles.infoLabel}>Author:</Text>
        <Text style={styles.infoValue}>jayis1</Text>
        <Text style={styles.infoLabel}>License:</Text>
        <Text style={styles.infoValue}>MIT (app) · GPL-2.0 (fw) · CERN-OHL-S v2 (hw)</Text>
      </View>

      <View style={styles.actions}>
        <TouchableOpacity style={styles.actionBtn} onPress={handlePing}>
          <Text style={styles.actionText}>Ping Device</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn} onPress={handleStatus}>
          <Text style={styles.actionText}>Get Status</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn} onPress={handleFwUpdate}>
          <Text style={styles.actionText}>Firmware Update (OTA)</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.rekeySection}>
        <Text style={styles.sectionTitle}>BLE Session Re-key</Text>
        <Text style={styles.rekeyDesc}>
          Trigger a new ECDH P-256 handshake. The session key is used for
          AES-256-CTR encryption of all C2 traffic.
        </Text>
        <TextInput
          style={styles.keyInput}
          value={newKey}
          onChangeText={setNewKey}
          placeholder="new key passphrase (min 8 chars)"
          placeholderTextColor="#6e7681"
          secureTextEntry
        />
        <TouchableOpacity style={styles.rekeyButton} onPress={handleRekey}>
          <Text style={styles.rekeyButtonText}>Re-key</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity style={styles.disconnectButton} onPress={handleDisconnect}>
        <Text style={styles.disconnectText}>Disconnect</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>© 2026 jayis1 — Authorized security research use only</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0d1117' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#8b949e', marginBottom: 16 },
  infoCard: { backgroundColor: '#161b22', padding: 14, borderRadius: 10, borderWidth: 1, borderColor: '#30363d', marginBottom: 16 },
  infoLabel: { color: '#6e7681', fontSize: 11, marginTop: 4 },
  infoValue: { color: '#f0f6fc', fontSize: 13 },
  actions: { gap: 8, marginBottom: 20 },
  actionBtn: { backgroundColor: '#21262d', padding: 12, borderRadius: 8, borderWidth: 1, borderColor: '#30363d' },
  actionText: { color: '#f0f6fc', fontSize: 14 },
  rekeySection: { backgroundColor: '#161b22', padding: 14, borderRadius: 10, borderWidth: 1, borderColor: '#30363d', marginBottom: 16 },
  sectionTitle: { color: '#f0f6fc', fontSize: 15, fontWeight: 'bold', marginBottom: 6 },
  rekeyDesc: { color: '#8b949e', fontSize: 11, marginBottom: 10 },
  keyInput: { backgroundColor: '#0d1117', color: '#f0f6fc', borderRadius: 6, paddingHorizontal: 10, height: 38, borderWidth: 1, borderColor: '#30363d', marginBottom: 10 },
  rekeyButton: { backgroundColor: '#a371f7', padding: 12, borderRadius: 8, alignItems: 'center' },
  rekeyButtonText: { color: '#fff', fontWeight: 'bold' },
  disconnectButton: { backgroundColor: '#f85149', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  disconnectText: { color: '#fff', fontWeight: 'bold', fontSize: 15 },
  footer: { color: '#6e7681', fontSize: 10, textAlign: 'center' },
});