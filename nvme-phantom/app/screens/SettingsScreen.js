/**
 * screens/SettingsScreen.js — App settings
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Switch, Alert } from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';

export default function SettingsScreen({ ble }) {
  const [bleKey, setBleKey] = useState('4A415959495331004E564D50484100');
  const [verboseLog, setVerboseLog] = useState(false);
  const [oledBright, setOledBright] = useState('128');
  const [deviceInfo, setDeviceInfo] = useState(null);

  const handlePing = async () => {
    try {
      const v = await ble.ping();
      Alert.alert('Ping', `Device responded. Version: 0x${v.toString(16)}`);
    } catch (e) { Alert.alert('Ping failed', e.message); }
  };

  const handleSaveKey = async () => {
    try {
      await AsyncStorage.setItem('ble_key', bleKey);
      // Convert hex string to byte array and set on BLE manager
      const keyBytes = new Uint8Array(16);
      for (let i = 0; i < 16; i++) {
        keyBytes[i] = parseInt(bleKey.substr(i * 2, 2), 16);
      }
      ble.key = keyBytes;
      Alert.alert('Saved', 'BLE encryption key updated.');
    } catch (e) { Alert.alert('Error', e.message); }
  };

  const handleDisconnect = async () => {
    try {
      await ble.disconnect();
      Alert.alert('Disconnected');
    } catch (e) { Alert.alert('Error', e.message); }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Settings</Text>

      <Text style={styles.label}>BLE Encryption Key (32-char hex = 16 bytes)</Text>
      <TextInput style={styles.input} value={bleKey} onChangeText={setBleKey}
        placeholder="4A415959495331004E564D50484100" autoCapitalize="none"/>
      <TouchableOpacity style={styles.button} onPress={handleSaveKey}>
        <Text style={styles.buttonText}>Save Key</Text>
      </TouchableOpacity>

      <View style={styles.row}>
        <Text style={styles.rowLabel}>Verbose Logging</Text>
        <Switch value={verboseLog} onValueChange={setVerboseLog} />
      </View>

      <Text style={styles.label}>OLED Brightness (0-255)</Text>
      <TextInput style={styles.input} value={oledBright} onChangeText={setOledBright} keyboardType="numeric"/>

      <Text style={styles.sectionTitle}>Device</Text>
      <TouchableOpacity style={styles.button} onPress={handlePing}>
        <Text style={styles.buttonText}>Ping Device</Text>
      </TouchableOpacity>

      <TouchableOpacity style={[styles.button, styles.buttonRed]} onPress={handleDisconnect}>
        <Text style={styles.buttonText}>Disconnect</Text>
      </TouchableOpacity>

      <Text style={styles.sectionTitle}>About</Text>
      <Text style={styles.about}>NVMe-Phantom Companion App v1.0</Text>
      <Text style={styles.about}>Author: jayis1</Text>
      <Text style={styles.about}>License: MIT</Text>
      <Text style={styles.about}>Device: M.2 NVMe / PCIe Gen3x4 Storage Interposer</Text>
      <Text style={styles.legal}>
        ⚠️ LEGAL: This tool is for authorized security research{'\n'}
        and penetration testing only. Unauthorized interception or{'\n'}
        modification of storage traffic may violate CFAA (18 USC §1030),{'\n'}
        EPCA, DMCA anti-circumvention (17 USC §1201), and state laws.{'\n'}
        Always obtain written authorization before deployment.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 15 },
  label: { color: '#aaa', fontSize: 14, marginTop: 10, marginBottom: 5 },
  input: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, padding: 10, fontSize: 13, fontFamily: 'monospace', marginBottom: 5 },
  button: { backgroundColor: '#00AA00', padding: 12, borderRadius: 6, alignItems: 'center', marginTop: 5 },
  buttonRed: { backgroundColor: '#cc3300' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 10 },
  rowLabel: { color: '#ccc', fontSize: 14 },
  sectionTitle: { fontSize: 18, color: '#888', marginTop: 20, marginBottom: 10 },
  about: { color: '#aaa', fontSize: 13, marginBottom: 3 },
  legal: { color: '#FF6600', fontSize: 10, marginTop: 15, lineHeight: 16 },
});