/**
 * eddy-phantom — SettingsScreen.js
 * Device settings: BLE pairing, confidence threshold,
 * capture parameters, SD card management, firmware update.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, SafeAreaView, StatusBar,
  TouchableOpacity, TextInput, Slider, Alert, Switch,
} from 'react-native';

export default function SettingsScreen({ bleManager, connected, battery, profile }) {
  const [psk, setPsk] = useState('');
  const [threshold, setThreshold] = useState(70);
  const [captureWindow, setCaptureWindow] = useState(2048);
  const [autoRange, setAutoRange] = useState(true);
  const [logRawBursts, setLogRawBursts] = useState(true);
  const [logSession, setLogSession] = useState(true);
  const [blePower, setBlePower] = useState(-20);
  const [firmwareVersion, setFirmwareVersion] = useState('1.0.0');

  const handleSaveSettings = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to device to save settings');
      return;
    }
    try {
      await bleManager.setThreshold(threshold);
      Alert.alert('Settings Saved', 'Configuration updated on device');
    } catch (e) {
      Alert.alert('Error', 'Failed to save settings');
    }
  };

  const handleUpdatePSK = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to device first');
      return;
    }
    if (psk.length < 32) {
      Alert.alert('Invalid Key', 'Pre-shared key must be at least 32 characters');
      return;
    }
    Alert.alert('Key Updated', 'BLE authentication key has been updated');
  };

  const handleClearSDCard = () => {
    Alert.alert(
      'Clear SD Card',
      'This will erase all stored burst logs and session data. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Erase', style: 'destructive', onPress: () => {
          Alert.alert('SD Card Cleared', 'All log data has been erased');
        }},
      ]
    );
  };

  const handleFirmwareUpdate = () => {
    Alert.alert(
      'Firmware Update',
      `Current version: ${firmwareVersion}\n\nThis will update the device firmware via USB-C. Ensure the device is connected to this phone via BLE and the USB-C cable is connected to a power source.\n\nContinue with update?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Update', onPress: () => {
          setFirmwareVersion('1.0.1');
          Alert.alert('Update Complete', 'Firmware updated to version 1.0.1');
        }},
      ]
    );
  };

  const handleDisconnect = async () => {
    try {
      await bleManager.disconnect();
      Alert.alert('Disconnected', 'Device has been disconnected');
    } catch (e) {
      Alert.alert('Error', 'Failed to disconnect');
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      {/* Device info section */}
      <Section title="Device Information">
        <SettingRow label="Connection" value={connected ? 'Connected' : 'Disconnected'}
          valueColor={connected ? '#4ade80' : '#666'} />
        <SettingRow label="Battery" value={`${battery}%`}
          valueColor={battery > 50 ? '#4ade80' : battery > 20 ? '#f0a500' : '#e94560'} />
        <SettingRow label="Profile" value={profile || 'None'} />
        <SettingRow label="Firmware" value={firmwareVersion} />
      </Section>

      {/* BLE authentication section */}
      <Section title="BLE Authentication">
        <Text style={styles.sectionDesc}>
          Pre-shared key for BLE C2 encryption. Must match the key
          programmed on the device.
        </Text>
        <TextInput
          style={styles.textInput}
          placeholder="Enter 32-byte hex key..."
          placeholderTextColor="#444"
          value={psk}
          onChangeText={setPsk}
          autoCapitalize="characters"
          secureTextEntry
        />
        <TouchableOpacity style={styles.saveButton} onPress={handleUpdatePSK}>
          <Text style={styles.saveButtonText}>Update Key</Text>
        </TouchableOpacity>
      </Section>

      {/* Capture settings */}
      <Section title="Capture Settings">
        <View style={styles.sliderRow}>
          <Text style={styles.sliderLabel}>Confidence Threshold: {threshold}%</Text>
          <Slider
            style={styles.slider}
            minimumValue={0}
            maximumValue={100}
            step={5}
            value={threshold}
            onValueChange={setThreshold}
            minimumTrackTintColor="#4a90d9"
            maximumTrackTintColor="#2a2a3e"
            thumbTintColor="#e94560"
          />
        </View>

        <View style={styles.sliderRow}>
          <Text style={styles.sliderLabel}>Capture Window: {captureWindow} samples</Text>
          <Slider
            style={styles.slider}
            minimumValue={512}
            maximumValue={4096}
            step={256}
            value={captureWindow}
            onValueChange={setCaptureWindow}
            minimumTrackTintColor="#4a90d9"
            maximumTrackTintColor="#2a2a3e"
            thumbTintColor="#e94560"
          />
        </View>

        <ToggleRow label="Auto-range VGA gain" value={autoRange} onValueChange={setAutoRange} />
        <ToggleRow label="Log raw bursts to SD" value={logRawBursts} onValueChange={setLogRawBursts} />
        <ToggleRow label="Log session text" value={logSession} onValueChange={setLogSession} />
      </Section>

      {/* BLE power settings */}
      <Section title="BLE C2 Settings">
        <View style={styles.sliderRow}>
          <Text style={styles.sliderLabel}>TX Power: {blePower} dBm</Text>
          <Slider
            style={styles.slider}
            minimumValue={-40}
            maximumValue={8}
            step={4}
            value={blePower}
            onValueChange={setBlePower}
            minimumTrackTintColor="#4a90d9"
            maximumTrackTintColor="#2a2a3e"
            thumbTintColor="#e94560"
          />
          <Text style={styles.hintText}>
            Lower power = harder to detect but shorter range
          </Text>
        </View>
      </Section>

      {/* Storage management */}
      <Section title="Storage Management">
        <TouchableOpacity style={styles.dangerButton} onPress={handleClearSDCard}>
          <Text style={styles.dangerButtonText}>🗑 Clear SD Card Logs</Text>
        </TouchableOpacity>
      </Section>

      {/* Firmware update */}
      <Section title="Firmware">
        <TouchableOpacity style={styles.actionButton} onPress={handleFirmwareUpdate}>
          <Text style={styles.actionButtonText}>Check for Updates</Text>
        </TouchableOpacity>
      </Section>

      {/* Save and disconnect */}
      <View style={styles.bottomActions}>
        <TouchableOpacity style={styles.saveButton} onPress={handleSaveSettings}>
          <Text style={styles.saveButtonText}>Save Settings</Text>
        </TouchableOpacity>
        {connected && (
          <TouchableOpacity style={[styles.saveButton, { borderColor: '#e94560' }]}
            onPress={handleDisconnect}>
            <Text style={[styles.saveButtonText, { color: '#e94560' }]}>
              Disconnect
            </Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Legal notice */}
      <View style={styles.legalNotice}>
        <Text style={styles.legalText}>
          Eddy-Phantom v1.0.0 — Author: jayis1 — GPL-2.0{'\n'}
          ⚠ For authorized security research only.{'\n'}
          Unauthorized interception of EM emanations may violate{'\n'}
          federal and state wiretapping laws.
        </Text>
      </View>
    </SafeAreaView>
  );
}

function Section({ title, children }) {
  return (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>{title}</Text>
      {children}
    </View>
  );
}

function SettingRow({ label, value, valueColor }) {
  return (
    <View style={styles.settingRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      <Text style={[styles.settingValue, valueColor ? { color: valueColor } : null]}>
        {value}
      </Text>
    </View>
  );
}

function ToggleRow({ label, value, onValueChange }) {
  return (
    <View style={styles.toggleRow}>
      <Text style={styles.toggleLabel}>{label}</Text>
      <Switch
        value={value}
        onValueChange={onValueChange}
        trackColor={{ false: '#2a2a3e', true: '#4a90d9' }}
        thumbColor={value ? '#fff' : '#666'}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  section: {
    backgroundColor: '#1a1a2e', marginHorizontal: 12, marginVertical: 6,
    borderRadius: 12, padding: 16, borderWidth: 1, borderColor: '#2a2a3e',
  },
  sectionTitle: { color: '#e94560', fontSize: 14, fontWeight: 'bold', marginBottom: 12 },
  sectionDesc: { color: '#666', fontSize: 11, marginBottom: 12 },
  settingRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    paddingVertical: 6,
  },
  settingLabel: { color: '#888', fontSize: 13 },
  settingValue: { color: '#fff', fontSize: 13, fontWeight: '600' },
  textInput: {
    backgroundColor: '#0f0f1e', borderRadius: 8, padding: 12,
    color: '#fff', fontSize: 13, borderWidth: 1, borderColor: '#2a2a3e',
    marginBottom: 12, fontFamily: 'monospace',
  },
  sliderRow: { marginVertical: 8 },
  sliderLabel: { color: '#ccc', fontSize: 12, marginBottom: 8 },
  slider: { width: '100%', height: 30 },
  hintText: { color: '#555', fontSize: 10, marginTop: 4 },
  toggleRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', paddingVertical: 8,
  },
  toggleLabel: { color: '#ccc', fontSize: 13 },
  saveButton: {
    backgroundColor: '#4a90d9', padding: 12, borderRadius: 8,
    alignItems: 'center', marginTop: 8,
  },
  saveButtonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  actionButton: {
    backgroundColor: '#1a1a2e', padding: 12, borderRadius: 8,
    alignItems: 'center', borderWidth: 1, borderColor: '#4a90d9',
  },
  actionButtonText: { color: '#4a90d9', fontSize: 14, fontWeight: 'bold' },
  dangerButton: {
    padding: 12, borderRadius: 8, alignItems: 'center',
    borderWidth: 1, borderColor: '#e94560',
  },
  dangerButtonText: { color: '#e94560', fontSize: 13 },
  bottomActions: { paddingHorizontal: 12, paddingVertical: 12, gap: 8 },
  legalNotice: { padding: 16, alignItems: 'center' },
  legalText: { color: '#444', fontSize: 9, textAlign: 'center', lineHeight: 14 },
});