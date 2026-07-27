// src/screens/SettingsScreen.tsx — App settings and device configuration
//
// Author: jayis1
// License: GPL-2.0

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, TextInput, Alert, Linking } from 'react-native';
import { useBle } from '../ble/BleManager';
import { CMD } from '../types';

export default function SettingsScreen() {
  const { connected, sendCommand, deviceName, disconnect, error } = useBle();
  const [tamperThreshold, setTamperThreshold] = useState('1500');
  const [gnssDiscipline, setGnssDiscipline] = useState(false);

  const setTamper = async () => {
    if (!connected) return;
    const mg = parseInt(tamperThreshold) || 1500;
    const payload = new Uint8Array(2);
    payload[0] = (mg >> 8) & 0xFF;
    payload[1] = mg & 0xFF;
    await sendCommand(CMD.TAMPER_THRESHOLD, payload);
    Alert.alert('Tamper Threshold Set', `${mg} mg`);
  };

  const toggleGnss = async () => {
    if (!connected) return;
    const newState = !gnssDiscipline;
    setGnssDiscipline(newState);
    const payload = new Uint8Array(1);
    payload[0] = newState ? 1 : 0;
    await sendCommand(CMD.GNSS_DISCIPLINE, payload);
  };

  const firmwareUpdate = () => {
    Alert.alert(
      'Firmware Update',
      'Firmware updates are delivered via USB-C. Connect the device to a computer and use:\n\nopenocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program chronos-phantom.bin 0x08000000 verify reset exit"',
      [{ text: 'OK' }]
    );
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Connection</Text>
          <Text style={styles.infoValue}>{connected ? 'Connected' : 'Disconnected'}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Device Name</Text>
          <Text style={styles.infoValue}>{deviceName || '—'}</Text>
        </View>
        {error && (
          <Text style={styles.errorText}>{error}</Text>
        )}
        {connected && (
          <TouchableOpacity style={styles.buttonDanger} onPress={disconnect}>
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Tamper Detection</Text>
        <Text style={styles.sectionDesc}>
          IMU-based tamper detection triggers zeroization if the device
          is physically moved beyond the threshold (in milli-g).
        </Text>
        <View style={styles.inputRow}>
          <TextInput
            style={styles.input}
            value={tamperThreshold}
            keyboardType="numeric"
            onChangeText={setTamperThreshold}
          />
          <Text style={styles.inputUnit}>mg</Text>
        </View>
        <TouchableOpacity style={styles.buttonSecondary} onPress={setTamper}>
          <Text style={styles.buttonText}>Set Threshold</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>GNSS Disciplining</Text>
        <Text style={styles.sectionDesc}>
          When enabled, the TCXO is disciplined to the GNSS 1-PPS signal,
          providing a legitimate time reference for grandmaster spoofing.
        </Text>
        <TouchableOpacity
          style={[styles.toggleButton, gnssDiscipline ? styles.toggleOn : styles.toggleOff]}
          onPress={toggleGnss}
        >
          <Text style={styles.buttonText}>
            {gnssDiscipline ? 'Disciplining: ON' : 'Disciplining: OFF'}
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <Text style={styles.sectionDesc}>Version 1.0.0 — Author: jayis1</Text>
        <TouchableOpacity style={styles.buttonSecondary} onPress={firmwareUpdate}>
          <Text style={styles.buttonText}>Firmware Update Info</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>
          Chronos-Phantom is an IEEE 1588 PTP / NTP time-synchronization
          attack platform designed for authorized security research and
          red-team operations.
        </Text>
        <Text style={styles.aboutText}>
          Author: jayis1{'\n'}
          License: GPL-2.0 (firmware & app) / CERN-OHL-S v2 (hardware)
        </Text>
        <Text style={styles.disclaimer}>
          ⚠️ This device is for authorized security research only.
          Unauthorized manipulation of time-synchronization infrastructure
          may violate computer-fraud, critical-infrastructure, and
          financial-services laws. Always obtain written authorization.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a' },
  content: { padding: 16, paddingBottom: 40 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#00ff88', marginBottom: 16 },
  section: { backgroundColor: '#1a1a1a', borderRadius: 12, padding: 16, marginBottom: 12 },
  sectionTitle: { fontSize: 14, color: '#00ff88', fontWeight: '600', marginBottom: 8 },
  sectionDesc: { fontSize: 11, color: '#888', marginBottom: 12 },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  infoLabel: { fontSize: 12, color: '#888' },
  infoValue: { fontSize: 12, color: '#ccc' },
  errorText: { fontSize: 11, color: '#ff4444', marginTop: 8 },
  inputRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  input: {
    flex: 1, backgroundColor: '#0a0a0a', borderRadius: 8, padding: 10,
    color: '#ccc', fontSize: 14, fontFamily: 'monospace',
    borderWidth: 1, borderColor: '#333',
  },
  inputUnit: { color: '#666', fontSize: 12, marginLeft: 8 },
  buttonDanger: { backgroundColor: '#aa3333', borderRadius: 8, padding: 12, alignItems: 'center', marginTop: 8 },
  buttonSecondary: { backgroundColor: '#333', borderRadius: 8, padding: 12, alignItems: 'center', marginTop: 8 },
  buttonText: { color: 'white', fontSize: 13, fontWeight: '600' },
  toggleButton: { borderRadius: 8, padding: 12, alignItems: 'center', marginTop: 8 },
  toggleOn: { backgroundColor: '#00aa44' },
  toggleOff: { backgroundColor: '#333' },
  aboutText: { fontSize: 11, color: '#888', marginBottom: 8, lineHeight: 18 },
  disclaimer: {
    fontSize: 10, color: '#ff8800', marginTop: 12, lineHeight: 16,
  },
});

// Author: jayis1
// License: GPL-2.0