/**
 * TeslaPhantom — Dashboard Screen
 * Shows device status, connection state, and quick actions.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const stateNames = [
  'BOOT', 'IDLE', 'ARMED', 'CHARGING', 'PULSE',
  'CAPTURE', 'ANALYZE', 'SCAN', 'CALIBRATE', 'FAULT', 'SHUTDOWN'
];

export default function DashboardScreen() {
  const { connected, status, scanning, startScan, sendCommand, error } = useDevice();

  const handleArm = () => {
    if (status.armed) {
      sendCommand({ cmd: 'emfi_disarm' });
    } else {
      sendCommand({
        cmd: 'emfi_arm',
        voltage: 200,
        width_ns: 50,
        delay_ns: 1000
      });
    }
  };

  const handleHome = () => {
    sendCommand({ cmd: 'home' });
  };

  const handleDischarge = () => {
    Alert.alert(
      'Discharge HV',
      'Discharge the Marx bank?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Discharge', onPress: () => sendCommand({ cmd: 'discharge' }) }
      ]
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>TeslaPhantom</Text>
        <Text style={styles.subtitle}>EMFI & Magnetic SCA Platform</Text>
        <Text style={styles.author}>by jayis1 · GPL-2.0</Text>
      </View>

      {!connected ? (
        <View style={styles.connectBox}>
          <Text style={styles.connectText}>Not connected</Text>
          <TouchableOpacity style={styles.button} onPress={startScan}>
            <Text style={styles.buttonText}>
              {scanning ? 'Scanning...' : 'Scan for Device'}
            </Text>
          </TouchableOpacity>
          {error && <Text style={styles.errorText}>{error}</Text>}
        </View>
      ) : (
        <View style={styles.statusBox}>
          <View style={styles.statusRow}>
            <Text style={styles.label}>State:</Text>
            <Text style={[styles.value, status.armed && styles.armed]}>
              {stateNames[status.state] || 'UNKNOWN'}
            </Text>
          </View>

          <View style={styles.statusRow}>
            <Text style={styles.label}>Battery:</Text>
            <Text style={styles.value}>{status.battery}%</Text>
          </View>

          <View style={styles.statusRow}>
            <Text style={styles.label}>Mode:</Text>
            <Text style={styles.value}>{status.mode}</Text>
          </View>

          <View style={styles.statusRow}>
            <Text style={styles.label}>HV:</Text>
            <Text style={[styles.value, status.hvVoltage > 0 && styles.armed]}>
              {status.hvVoltage}V
            </Text>
          </View>

          <View style={styles.statusRow}>
            <Text style={styles.label}>Position:</Text>
            <Text style={styles.value}>
              X:{status.position.x.toFixed(2)} Y:{status.position.y.toFixed(2)} Z:{status.position.z.toFixed(2)}
            </Text>
          </View>

          <View style={styles.buttonRow}>
            <TouchableOpacity
              style={[styles.button, status.armed ? styles.disarmBtn : styles.armBtn]}
              onPress={handleArm}
            >
              <Text style={styles.buttonText}>
                {status.armed ? 'DISARM' : 'ARM'}
              </Text>
            </TouchableOpacity>

            <TouchableOpacity style={styles.button} onPress={handleHome}>
              <Text style={styles.buttonText}>HOME</Text>
            </TouchableOpacity>

            <TouchableOpacity style={[styles.button, styles.dangerBtn]} onPress={handleDischarge}>
              <Text style={styles.buttonText}>DISCHARGE</Text>
            </TouchableOpacity>
          </View>
        </View>
      )}

      <View style={styles.warningBox}>
        <Text style={styles.warningTitle}>⚠ HIGH VOLTAGE</Text>
        <Text style={styles.warningText}>
          This device generates up to 400V. Use safety interlock.
          For authorized security research only.
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  header: { alignItems: 'center', marginBottom: 20, marginTop: 10 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#e74c3c' },
  subtitle: { fontSize: 14, color: '#95a5a6', marginTop: 4 },
  author: { fontSize: 11, color: '#555', marginTop: 2 },
  connectBox: { alignItems: 'center', marginTop: 40 },
  connectText: { fontSize: 18, color: '#95a5a6', marginBottom: 20 },
  statusBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginBottom: 20 },
  statusRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8 },
  label: { fontSize: 16, color: '#95a5a6' },
  value: { fontSize: 16, color: '#fff', fontWeight: 'bold' },
  armed: { color: '#e74c3c' },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 20 },
  button: { backgroundColor: '#2c3e50', paddingHorizontal: 20, paddingVertical: 12, borderRadius: 6 },
  armBtn: { backgroundColor: '#e74c3c' },
  disarmBtn: { backgroundColor: '#27ae60' },
  dangerBtn: { backgroundColor: '#c0392b' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  errorText: { color: '#e74c3c', fontSize: 12, marginTop: 10, textAlign: 'center' },
  warningBox: { backgroundColor: '#2c1010', borderRadius: 8, padding: 12, marginTop: 'auto' },
  warningTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  warningText: { color: '#aaa', fontSize: 11 },
});