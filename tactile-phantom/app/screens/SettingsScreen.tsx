/**
 * Tactile-Phantom — Companion App
 * screens/SettingsScreen.tsx — Device settings, firmware update, diagnostics
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet, Alert, Switch,
} from 'react-native';
import { bleManager } from '../src/ble';
import { useStore, VENDOR_NAMES } from '../src/store';

export default function SettingsScreen() {
  const status = useStore((s) => s.status);
  const connected = useStore((s) => s.connected);
  const setConnected = useStore((s) => s.setConnected);

  const [busMode, setBusMode] = useState(status?.mode || 0);
  const [captureEnabled, setCaptureEnabled] = useState(true);
  const [debugLogging, setDebugLogging] = useState(false);
  const [firmwareVersion, setFirmwareVersion] = useState('1.0.0');

  const disconnect = async () => {
    Alert.alert(
      'Disconnect',
      'Disconnect from Tactile-Phantom?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Disconnect',
          style: 'destructive',
          onPress: async () => {
            await bleManager.disconnect();
            setConnected(false);
          },
        },
      ]
    );
  };

  const setMode = async (mode: number) => {
    setBusMode(mode);
    // Send mode change command to device
    Alert.alert('Mode Set', `Bus mode: ${mode === 0 ? 'Auto' : mode === 1 ? 'I2C' : 'SPI'}`);
  };

  const firmwareUpdate = () => {
    Alert.alert(
      'Firmware Update',
      `Current: v${firmwareVersion}\nCheck for updates over WiFi?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Check & Update',
          onPress: () => {
            Alert.alert('Checking', 'Querying ESP32-C3 for OTA updates...');
            // In production: HTTP GET to device's WiFi AP for firmware info
          },
        },
      ]
    );
  };

  const runDiagnostics = () => {
    Alert.alert(
      'Diagnostics',
      `Connection: ${connected ? 'Connected' : 'Disconnected'}\n` +
      `Vendor: ${status ? VENDOR_NAMES[status.vendor] : 'N/A'}\n` +
      `Bus: ${status ? (status.mode === 1 ? 'I2C' : 'SPI') : 'N/A'}\n` +
      `Address: 0x${status?.i2cAddr.toString(16) || '??'}\n` +
      `Clock: ${status?.clockHz || 0} Hz\n` +
      `Resolution: ${status?.xResolution || 0}×${status?.yResolution || 0}\n` +
      `Transactions: ${status?.totalTransactions || 0}\n` +
      `Events: ${status?.totalEvents || 0}\n` +
      `Battery: ${status ? (status.batteryMv / 1000).toFixed(2) : 0}V\n` +
      `Firmware: v${firmwareVersion}`,
      [{ text: 'OK' }]
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      {/* Connection info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <Text style={styles.infoRow}>Status: {connected ? '● Connected' : '○ Disconnected'}</Text>
        {status && (
          <>
            <Text style={styles.infoRow}>Vendor: {VENDOR_NAMES[status.vendor] || 'Unknown'}</Text>
            <Text style={styles.infoRow}>Bus: {status.mode === 1 ? 'I2C' : 'SPI'} @0x{status.i2cAddr.toString(16)}</Text>
            <Text style={styles.infoRow}>Clock: {status.clockHz} Hz</Text>
            <Text style={styles.infoRow}>Resolution: {status.xResolution}×{status.yResolution}</Text>
            <Text style={styles.infoRow}>Battery: {(status.batteryMv / 1000).toFixed(2)}V</Text>
          </>
        )}
        <TouchableOpacity style={[styles.btn, { borderColor: '#cc3333' }]} onPress={disconnect}>
          <Text style={[styles.btnText, { color: '#cc8888' }]}>Disconnect</Text>
        </TouchableOpacity>
      </View>

      {/* Bus configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Bus Configuration</Text>
        <Text style={styles.settingLabel}>Bus Mode</Text>
        <View style={styles.modeRow}>
          {[
            { label: 'Auto', value: 0 },
            { label: 'I2C', value: 1 },
            { label: 'SPI', value: 2 },
          ].map((m) => (
            <TouchableOpacity
              key={m.value}
              style={[styles.modeBtn, busMode === m.value && styles.modeBtnActive]}
              onPress={() => setMode(m.value)}
            >
              <Text style={[styles.modeBtnText, busMode === m.value && styles.modeBtnTextActive]}>
                {m.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Capture settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Capture</Text>
        <View style={styles.switchRow}>
          <Text style={styles.settingLabel}>Capture Enabled</Text>
          <Switch
            value={captureEnabled}
            onValueChange={setCaptureEnabled}
            trackColor={{ false: '#333', true: '#00ff88' }}
            thumbColor={captureEnabled ? '#fff' : '#888'}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.settingLabel}>Debug Logging</Text>
          <Switch
            value={debugLogging}
            onValueChange={setDebugLogging}
            trackColor={{ false: '#333', true: '#ffaa00' }}
            thumbColor={debugLogging ? '#fff' : '#888'}
          />
        </View>
      </View>

      {/* Firmware */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <Text style={styles.infoRow}>Version: v{firmwareVersion}</Text>
        <Text style={styles.infoRow}>Author: jayis1</Text>
        <Text style={styles.infoRow}>License: GPL-2.0</Text>
        <TouchableOpacity style={styles.btn} onPress={firmwareUpdate}>
          <Text style={styles.btnText}>Check for Updates</Text>
        </TouchableOpacity>
      </View>

      {/* Diagnostics */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Diagnostics</Text>
        <TouchableOpacity style={styles.btn} onPress={runDiagnostics}>
          <Text style={styles.btnText}>Run Diagnostics</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.aboutSection}>
        <Text style={styles.aboutTitle}>Tactile-Phantom v1.0.0</Text>
        <Text style={styles.aboutAuthor}>Author: jayis1</Text>
        <Text style={styles.aboutLicense}>License: GPL-2.0</Text>
        <Text style={styles.aboutDesc}>
          Covert touch-controller bus MITM implant for authorized security research.
        </Text>
        <Text style={styles.aboutWarning}>
          ⚠️ For authorized security research only.{'\n'}
          Obtain written consent before deployment.
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 15 },
  title: { color: '#e0e0e0', fontSize: 22, fontWeight: 'bold', textAlign: 'center', marginBottom: 15 },
  section: { backgroundColor: '#16213e', borderRadius: 10, padding: 12, marginBottom: 12, borderWidth: 1, borderColor: '#0f3460' },
  sectionTitle: { color: '#00ff88', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  infoRow: { color: '#aaa', fontSize: 12, marginBottom: 4 },
  settingLabel: { color: '#e0e0e0', fontSize: 13 },
  switchRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginBottom: 8 },
  modeRow: { flexDirection: 'row', gap: 8, marginTop: 5 },
  modeBtn: { flex: 1, backgroundColor: '#0a0a14', borderRadius: 6, padding: 8, alignItems: 'center', borderWidth: 1, borderColor: '#1a1a2e' },
  modeBtnActive: { borderColor: '#00ff88', backgroundColor: '#0a2a1a' },
  modeBtnText: { color: '#888', fontSize: 12 },
  modeBtnTextActive: { color: '#00ff88' },
  btn: { backgroundColor: '#0a2a1a', borderRadius: 8, padding: 10, alignItems: 'center', borderWidth: 1, borderColor: '#00ff88', marginTop: 8 },
  btnText: { color: '#00ff88', fontSize: 13, fontWeight: 'bold' },
  aboutSection: { backgroundColor: '#16213e', borderRadius: 10, padding: 15, marginBottom: 20, borderWidth: 1, borderColor: '#0f3460', alignItems: 'center' },
  aboutTitle: { color: '#e0e0e0', fontSize: 16, fontWeight: 'bold', marginBottom: 5 },
  aboutAuthor: { color: '#888', fontSize: 12 },
  aboutLicense: { color: '#888', fontSize: 12 },
  aboutDesc: { color: '#aaa', fontSize: 11, textAlign: 'center', marginTop: 8, marginBottom: 8 },
  aboutWarning: { color: '#cc8888', fontSize: 10, textAlign: 'center' },
});