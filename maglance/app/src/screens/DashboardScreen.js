/**
 * DashboardScreen.js — Real-time status display for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Shows connection status, system mode, coil current, battery voltage,
 * temperatures, 3-axis field readings from all three magnetometers,
 * active profile, and connected coil tip.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import { useBLE } from '../context/BLEContext';

export default function DashboardScreen() {
  const { connectionState, telemetry, scanForDevices, scanResults,
          connectToDevice, disconnect, sendCommand } = useBLE();
  const [showScanResults, setShowScanResults] = useState(false);

  const handleScan = async () => {
    await scanForDevices();
    setShowScanResults(true);
  };

  const handleConnect = async (deviceId) => {
    setShowScanResults(false);
    await connectToDevice(deviceId);
  };

  const handleEmergencyStop = () => {
    Alert.alert(
      'Emergency Stop',
      'Send STOP command to MagLance?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'STOP', style: 'destructive', onPress: () => sendCommand('STOP\n') },
      ]
    );
  };

  const formatUT = (centiUT) => {
    const ut = centiUT / 100.0;
    return ut.toFixed(1) + ' µT';
  };

  const getConnectionColor = () => {
    switch (connectionState) {
      case 'connected': return '#2ecc71';
      case 'connecting': return '#f39c12';
      case 'scanning': return '#3498db';
      case 'error': return '#e74c3c';
      default: return '#95a5a6';
    }
  };

  return (
    <View style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>MagLance</Text>
        <View style={[styles.statusDot, { backgroundColor: getConnectionColor() }]} />
        <Text style={styles.statusText}>{connectionState}</Text>
      </View>

      {/* Connection controls */}
      {connectionState !== 'connected' && (
        <View style={styles.section}>
          <TouchableOpacity style={styles.button} onPress={handleScan}>
            <Text style={styles.buttonText}>Scan for Devices</Text>
          </TouchableOpacity>

          {showScanResults && scanResults.map((dev, idx) => (
            <TouchableOpacity
              key={idx}
              style={styles.deviceButton}
              onPress={() => handleConnect(dev.id)}
            >
              <Text style={styles.deviceName}>{dev.name || 'Unknown'}</Text>
              <Text style={styles.deviceId}>{dev.id}</Text>
            </TouchableOpacity>
          ))}
        </View>
      )}

      {connectionState === 'connected' && (
        <>
          {/* System status */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>System Status</Text>
            <View style={styles.statusGrid}>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Mode</Text>
                <Text style={styles.statusValue}>{telemetry.mode}</Text>
              </View>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Current</Text>
                <Text style={styles.statusValue}>{telemetry.current} mA</Text>
              </View>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Battery</Text>
                <Text style={[
                  styles.statusValue,
                  { color: telemetry.vbat < 3200 ? '#e74c3c' : '#2ecc71' }
                ]}>
                  {(telemetry.vbat / 1000).toFixed(2)} V
                </Text>
              </View>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Profile</Text>
                <Text style={styles.statusValue}>#{telemetry.profile}</Text>
              </View>
            </View>
          </View>

          {/* Temperature */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Temperature</Text>
            <View style={styles.tempRow}>
              <Text style={styles.tempLabel}>Bridge: </Text>
              <Text style={[
                styles.tempValue,
                { color: telemetry.tempBridge > 70 ? '#e74c3c' : '#ffffff' }
              ]}>
                {telemetry.tempBridge}°C
              </Text>
              <Text style={styles.tempLabel}>  Coil: </Text>
              <Text style={[
                styles.tempValue,
                { color: telemetry.tempCoil > 70 ? '#e74c3c' : '#ffffff' }
              ]}>
                {telemetry.tempCoil}°C
              </Text>
            </View>
          </View>

          {/* Magnetometer readings */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Magnetic Field (3 Sensors)</Text>
            {[0, 1, 2].map(i => (
              <View key={i} style={styles.sensorRow}>
                <Text style={styles.sensorLabel}>S{i} ({i * 15}mm):</Text>
                <Text style={styles.sensorValue}>
                  X: {formatUT(telemetry.fieldX[i])}  Y: {formatUT(telemetry.fieldY[i])}  Z: {formatUT(telemetry.fieldZ[i])}
                </Text>
              </View>
            ))}
          </View>

          {/* Error display */}
          {telemetry.error && (
            <View style={styles.errorBox}>
              <Text style={styles.errorText}>⚠ {telemetry.error}</Text>
            </View>
          )}

          {/* Emergency stop */}
          <TouchableOpacity style={styles.emergencyButton} onPress={handleEmergencyStop}>
            <Text style={styles.emergencyButtonText}>EMERGENCY STOP</Text>
          </TouchableOpacity>

          {/* Disconnect */}
          <TouchableOpacity style={styles.disconnectButton} onPress={disconnect}>
            <Text style={styles.disconnectButtonText}>Disconnect</Text>
          </TouchableOpacity>
        </>
      )}

      {/* Footer */}
      <View style={styles.footer}>
        <Text style={styles.footerText}>MagLance v1.0 — Author: jayis1</Text>
        <Text style={styles.footerWarning}>
          ⚠ For authorized security research only
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  header: { flexDirection: 'row', alignItems: 'center', marginBottom: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#e74c3c', marginRight: 8 },
  statusDot: { width: 10, height: 10, borderRadius: 5, marginRight: 6 },
  statusText: { color: '#95a5a6', fontSize: 12 },
  section: { backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 12 },
  sectionTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  statusGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  statusItem: { width: '48%', marginBottom: 8 },
  statusLabel: { color: '#95a5a6', fontSize: 11 },
  statusValue: { color: '#ffffff', fontSize: 16, fontWeight: 'bold' },
  tempRow: { flexDirection: 'row', alignItems: 'center' },
  tempLabel: { color: '#95a5a6', fontSize: 14 },
  tempValue: { color: '#ffffff', fontSize: 14, fontWeight: 'bold' },
  sensorRow: { flexDirection: 'row', marginBottom: 4 },
  sensorLabel: { color: '#95a5a6', fontSize: 12, width: 100 },
  sensorValue: { color: '#2ecc71', fontSize: 12, fontFamily: 'monospace' },
  errorBox: { backgroundColor: '#c0392b', borderRadius: 8, padding: 8, marginBottom: 12 },
  errorText: { color: '#ffffff', fontSize: 14 },
  button: { backgroundColor: '#e74c3c', padding: 12, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  buttonText: { color: '#ffffff', fontSize: 16, fontWeight: 'bold' },
  deviceButton: { backgroundColor: '#1a1a2e', padding: 12, borderRadius: 8, marginBottom: 4, borderWidth: 1, borderColor: '#3498db' },
  deviceName: { color: '#ffffff', fontSize: 14, fontWeight: 'bold' },
  deviceId: { color: '#95a5a6', fontSize: 10 },
  emergencyButton: { backgroundColor: '#c0392b', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  emergencyButtonText: { color: '#ffffff', fontSize: 18, fontWeight: 'bold' },
  disconnectButton: { backgroundColor: '#34495e', padding: 12, borderRadius: 8, alignItems: 'center' },
  disconnectButtonText: { color: '#ffffff', fontSize: 14 },
  footer: { marginTop: 'auto', paddingTop: 16 },
  footerText: { color: '#7f8c8d', fontSize: 10, textAlign: 'center' },
  footerWarning: { color: '#e67e22', fontSize: 10, textAlign: 'center', marginTop: 4 },
});