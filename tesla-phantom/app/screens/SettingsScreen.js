/**
 * TeslaPhantom — Settings Screen
 * BLE pairing, trigger source, safety interlocks, firmware update.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Switch, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function SettingsScreen() {
  const { connected, disconnect, sendCommand, status } = useDevice();
  const [safetyInterlock, setSafetyInterlock] = useState(true);
  const [autoDischarge, setAutoDischarge] = useState(true);
  const [blePsk, setBlePsk] = useState('');
  const [triggerSource, setTriggerSource] = useState(0);

  const triggerSources = ['External (SMA)', 'Internal Timer', 'Magnetic Signature', 'Manual'];

  const handleDisconnect = () => {
    Alert.alert('Disconnect', 'Disconnect from TeslaPhantom?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Disconnect', onPress: disconnect }
    ]);
  };

  const handleSetTrigger = (src) => {
    setTriggerSource(src);
    sendCommand({ cmd: 'set_trigger', source: src });
  };

  const handleAuthenticate = () => {
    if (blePsk.length < 6) {
      Alert.alert('Error', 'PSK must be at least 6 characters');
      return;
    }
    sendCommand({ cmd: 'authenticate', psk: blePsk });
    Alert.alert('Authentication', 'PSK sent to device');
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Connection</Text>

      <View style={styles.row}>
        <Text style={styles.label}>Status:</Text>
        <Text style={[styles.value, connected ? styles.connected : styles.disconnected]}>
          {connected ? 'Connected' : 'Disconnected'}
        </Text>
      </View>

      {connected && (
        <TouchableOpacity style={[styles.button, styles.disconnectBtn]} onPress={handleDisconnect}>
          <Text style={styles.buttonText}>Disconnect</Text>
        </TouchableOpacity>
      )}

      <Text style={styles.sectionTitle}>Trigger Source</Text>

      {triggerSources.map((src, idx) => (
        <TouchableOpacity
          key={idx}
          style={[styles.triggerRow, triggerSource === idx && styles.triggerRowActive]}
          onPress={() => handleSetTrigger(idx)}
        >
          <Text style={[styles.triggerText, triggerSource === idx && styles.triggerTextActive]}>
            {src}
          </Text>
        </TouchableOpacity>
      ))}

      <Text style={styles.sectionTitle}>Safety</Text>

      <View style={styles.switchRow}>
        <Text style={styles.label}>Safety Interlock</Text>
        <Switch
          value={safetyInterlock}
          onValueChange={setSafetyInterlock}
          trackColor={{ false: '#333', true: '#27ae60' }}
          thumbColor={safetyInterlock ? '#fff' : '#666'}
        />
      </View>

      <View style={styles.switchRow}>
        <Text style={styles.label}>Auto Discharge</Text>
        <Switch
          value={autoDischarge}
          onValueChange={setAutoDischarge}
          trackColor={{ false: '#333', true: '#27ae60' }}
          thumbColor={autoDischarge ? '#fff' : '#666'}
        />
      </View>

      <Text style={styles.sectionTitle}>BLE Authentication</Text>

      <TextInput
        style={styles.pskInput}
        value={blePsk}
        onChangeText={setBlePsk}
        placeholder="Enter BLE PSK (min 6 chars)"
        placeholderTextColor="#555"
        secureTextEntry
      />
      <TouchableOpacity style={[styles.button, styles.authBtn]} onPress={handleAuthenticate}>
        <Text style={styles.buttonText}>Authenticate</Text>
      </TouchableOpacity>

      <Text style={styles.sectionTitle}>Device Info</Text>

      <View style={styles.infoBox}>
        <Text style={styles.infoText}>Device: TeslaPhantom</Text>
        <Text style={styles.infoText}>Author: jayis1</Text>
        <Text style={styles.infoText}>License: GPL-2.0</Text>
        <Text style={styles.infoText}>Firmware: v1.0</Text>
        <Text style={styles.infoText}>MCU: STM32H743 @ 480 MHz</Text>
        <Text style={styles.infoText}>FPGA: iCE40UP5K</Text>
        <Text style={styles.infoText}>HV: 50–400V Marx Bank</Text>
        <Text style={styles.infoText}>ADC: AD7606C-6 (16-bit, 800 kSPS)</Text>
        <Text style={styles.infoText}>Sensor: DRV425 Fluxgate</Text>
      </View>

      <View style={styles.disclaimerBox}>
        <Text style={styles.disclaimerTitle}>⚠ Legal Disclaimer</Text>
        <Text style={styles.disclaimerText}>
          This device is a security research tool for authorized use only.
          Unauthorized use to attack systems you do not own or have permission
          to test is illegal. User is responsible for compliance with all
          applicable laws. Author: jayis1. Use responsibly.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#e74c3c', marginTop: 20, marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8 },
  label: { fontSize: 14, color: '#bbb' },
  value: { fontSize: 14, fontWeight: 'bold' },
  connected: { color: '#27ae60' },
  disconnected: { color: '#e74c3c' },
  button: { paddingVertical: 12, borderRadius: 6, alignItems: 'center', marginTop: 8 },
  disconnectBtn: { backgroundColor: '#c0392b' },
  authBtn: { backgroundColor: '#2980b9' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  triggerRow: { backgroundColor: '#1a1a2e', paddingVertical: 12, paddingHorizontal: 16,
                borderRadius: 4, marginBottom: 4, borderWidth: 1, borderColor: '#333' },
  triggerRowActive: { backgroundColor: '#e74c3c', borderColor: '#e74c3c' },
  triggerText: { color: '#95a5a6', fontSize: 14 },
  triggerTextActive: { color: '#fff', fontWeight: 'bold' },
  switchRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 8 },
  pskInput: { backgroundColor: '#1a1a2e', color: '#fff', borderWidth: 1, borderColor: '#333',
              borderRadius: 4, paddingHorizontal: 12, paddingVertical: 10, marginBottom: 8 },
  infoBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginTop: 8 },
  infoText: { color: '#95a5a6', fontSize: 12, paddingVertical: 2 },
  disclaimerBox: { backgroundColor: '#2c1010', borderRadius: 6, padding: 12, marginTop: 16, marginBottom: 40 },
  disclaimerTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  disclaimerText: { color: '#aaa', fontSize: 11, lineHeight: 16 },
});