/**
 * SettingsScreen.js
 *
 * Device settings, connection management, encryption key configuration,
 * firmware update, power management, and export options.
 *
 * Author: jayis1
 * @license MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  TextInput,
  Switch,
  StyleSheet,
  ScrollView,
  Alert,
} from 'react-native';

const SettingsScreen = ({ protocol, connectionState, deviceStatus, onConnectDevice }) => {
  const [encryptionKey, setEncryptionKey] = useState('');
  const [powerMode, setPowerMode] = useState('normal');
  const [autoReconnect, setAutoReconnect] = useState(true);
  const [deviceName, setDeviceName] = useState('GLYPH-REAPER');

  const handleSetEncryptionKey = useCallback(() => {
    if (encryptionKey.length !== 64) {
      Alert.alert('Invalid Key', 'Encryption key must be 64 hex characters (32 bytes)');
      return;
    }
    const keyBytes = [];
    for (let i = 0; i < 64; i += 2) {
      keyBytes.push(parseInt(encryptionKey.substr(i, 2), 16));
    }
    protocol.sendCommand(0x0E, keyBytes); /* CMD_SET_ENCRYPTION_KEY */
    Alert.alert('Success', 'Encryption key updated');
  }, [encryptionKey, protocol]);

  const handleGenerateKey = useCallback(() => {
    const chars = '0123456789ABCDEF';
    let key = '';
    for (let i = 0; i < 64; i++) {
      key += chars[Math.floor(Math.random() * 16)];
    }
    setEncryptionKey(key);
  }, []);

  const handleFirmwareUpdate = useCallback(() => {
    Alert.alert(
      'Firmware Update',
      'This will update the GLYPH-REAPER firmware. The device will restart.',
      [
        { text: 'Cancel' },
        {
          text: 'Update',
          onPress: () => {
            protocol.sendCommand(0x0D); /* CMD_FIRMWARE_UPDATE */
            Alert.alert('Updating', 'Firmware update in progress...');
          },
        },
      ]
    );
  }, [protocol]);

  const handleFactoryReset = useCallback(() => {
    Alert.alert(
      'Factory Reset',
      'This will erase all settings and return the device to defaults. This cannot be undone.',
      [
        { text: 'Cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: () => {
            protocol.sendCommand(0x13); /* CMD_FACTORY_RESET */
          },
        },
      ]
    );
  }, [protocol]);

  const handleEraseHistory = useCallback(() => {
    Alert.alert(
      'Erase Frame History',
      'Delete all stored frame metadata from device flash?',
      [
        { text: 'Cancel' },
        {
          text: 'Erase',
          style: 'destructive',
          onPress: () => {
            protocol.sendCommand(0x11); /* CMD_ERASE_HISTORY */
            Alert.alert('Success', 'Frame history erased');
          },
        },
      ]
    );
  }, [protocol]);

  const handleGetDeviceInfo = useCallback(() => {
    protocol.sendCommand(0x12); /* CMD_GET_DEVICE_INFO */
  }, [protocol]);

  const formatBattery = (mv) => {
    if (mv === 0) return 'N/A';
    const percent = Math.round((mv / 3200) * 100);
    return `${mv}mV (${Math.min(100, percent)}%)`;
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>BLE</Text>
          <Text style={[styles.infoValue, 
            connectionState.bleConnected ? styles.connected : styles.disconnected]}>
            {connectionState.bleConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>USB</Text>
          <Text style={[styles.infoValue,
            connectionState.usbConnected ? styles.connected : styles.disconnected]}>
            {connectionState.usbConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Device</Text>
          <Text style={styles.infoValue}>{connectionState.deviceName || 'N/A'}</Text>
        </View>
      </View>

      {/* Device status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Status</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>State</Text>
          <Text style={styles.infoValue}>{deviceStatus.systemState}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Protocol</Text>
          <Text style={styles.infoValue}>{deviceStatus.protocol}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Resolution</Text>
          <Text style={styles.infoValue}>
            {deviceStatus.frameWidth}×{deviceStatus.frameHeight}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Battery</Text>
          <Text style={styles.infoValue}>{formatBattery(deviceStatus.batteryMv)}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Temperature</Text>
          <Text style={styles.infoValue}>{deviceStatus.temperature}°C</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Uptime</Text>
          <Text style={styles.infoValue}>
            {Math.floor(deviceStatus.uptime / 3600)}h {Math.floor((deviceStatus.uptime % 3600) / 60)}m
          </Text>
        </View>
        <TouchableOpacity style={styles.btn} onPress={handleGetDeviceInfo}>
          <Text style={styles.btnText}>Refresh Device Info</Text>
        </TouchableOpacity>
      </View>

      {/* Encryption */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Encryption (AES-256-GCM)</Text>
        <TextInput
          style={styles.keyInput}
          placeholder="64 hex characters (256-bit key)"
          placeholderTextColor="#5C6877"
          value={encryptionKey}
          onChangeText={setEncryptionKey}
          autoCapitalize="none"
          autoCorrect={false}
          fontFamily="monospace"
        />
        <View style={styles.btnRow}>
          <TouchableOpacity style={styles.btn} onPress={handleGenerateKey}>
            <Text style={styles.btnText}>🎲 Generate</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.btn} onPress={handleSetEncryptionKey}>
            <Text style={styles.btnText}>✓ Set Key</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Power management */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Power Management</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Auto-reconnect</Text>
          <Switch
            value={autoReconnect}
            onValueChange={setAutoReconnect}
            trackColor={{ false: '#1E242C', true: '#00D4AA' }}
            thumbColor={autoReconnect ? '#FFFFFF' : '#5C6877'}
          />
        </View>
        <View style={styles.powerModeRow}>
          {['normal', 'reduced', 'low'].map(mode => (
            <TouchableOpacity
              key={mode}
              style={[
                styles.powerModeBtn,
                powerMode === mode && styles.powerModeBtnActive,
              ]}
              onPress={() => {
                setPowerMode(mode);
                protocol.sendCommand(0x10, [mode === 'normal' ? 0 : 
                  mode === 'reduced' ? 1 : 2]);
              }}
            >
              <Text style={[
                styles.powerModeText,
                powerMode === mode && styles.powerModeTextActive,
              ]}>
                {mode.charAt(0).toUpperCase() + mode.slice(1)}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Data management */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Management</Text>
        <TouchableOpacity style={styles.btn} onPress={handleEraseHistory}>
          <Text style={[styles.btnText, { color: '#FF4757' }]}>
            🗑 Erase Frame History
          </Text>
        </TouchableOpacity>
      </View>

      {/* Firmware */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <TouchableOpacity style={styles.btn} onPress={handleFirmwareUpdate}>
          <Text style={styles.btnText}>📦 Check for Updates</Text>
        </TouchableOpacity>
      </View>

      {/* Factory reset */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Advanced</Text>
        <TouchableOpacity style={[styles.btn, styles.dangerBtn]} onPress={handleFactoryReset}>
          <Text style={[styles.btnText, { color: '#FF4757' }]}>
            ⚠ Factory Reset
          </Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.aboutSection}>
        <Text style={styles.aboutTitle}>GLYPH-REAPER</Text>
        <Text style={styles.aboutVersion}>Version 1.0.0 — Rev A</Text>
        <Text style={styles.aboutAuthor}>Author: jayis1</Text>
        <Text style={styles.aboutLicense}>
          Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
        </Text>
        <Text style={styles.aboutWarning}>
          ⚠ For authorized security research only.{'\n'}
          Unauthorized interception of display content{'\n'}
          may violate applicable laws.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E14',
  },
  section: {
    backgroundColor: '#131720',
    marginHorizontal: 12,
    marginVertical: 6,
    borderRadius: 10,
    padding: 16,
  },
  sectionTitle: {
    color: '#00D4AA',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 12,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  infoLabel: {
    color: '#5C6877',
    fontSize: 14,
  },
  infoValue: {
    color: '#E6EDF3',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  connected: {
    color: '#00D4AA',
  },
  disconnected: {
    color: '#FF4757',
  },
  keyInput: {
    backgroundColor: '#0A0E14',
    borderWidth: 1,
    borderColor: '#1E242C',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 10,
    color: '#E6EDF3',
    fontSize: 12,
    marginBottom: 8,
  },
  btnRow: {
    flexDirection: 'row',
  },
  btn: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#1E242C',
    borderRadius: 8,
    marginHorizontal: 4,
  },
  btnText: {
    color: '#E6EDF3',
    fontSize: 14,
    fontWeight: '600',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  switchLabel: {
    color: '#E6EDF3',
    fontSize: 14,
  },
  powerModeRow: {
    flexDirection: 'row',
    marginTop: 8,
  },
  powerModeBtn: {
    flex: 1,
    paddingVertical: 8,
    alignItems: 'center',
    backgroundColor: '#0A0E14',
    borderRadius: 6,
    marginHorizontal: 2,
    borderWidth: 1,
    borderColor: '#1E242C',
  },
  powerModeBtnActive: {
    borderColor: '#00D4AA',
    backgroundColor: '#00D4AA10',
  },
  powerModeText: {
    color: '#5C6877',
    fontSize: 13,
    fontWeight: '600',
  },
  powerModeTextActive: {
    color: '#00D4AA',
  },
  dangerBtn: {
    backgroundColor: '#FF475710',
    borderColor: '#FF4757',
    borderWidth: 1,
  },
  aboutSection: {
    padding: 24,
    alignItems: 'center',
  },
  aboutTitle: {
    color: '#00D4AA',
    fontSize: 20,
    fontWeight: '700',
  },
  aboutVersion: {
    color: '#5C6877',
    fontSize: 14,
    marginTop: 4,
  },
  aboutAuthor: {
    color: '#E6EDF3',
    fontSize: 14,
    marginTop: 4,
  },
  aboutLicense: {
    color: '#5C6877',
    fontSize: 11,
    marginTop: 4,
    textAlign: 'center',
  },
  aboutWarning: {
    color: '#FF4757',
    fontSize: 11,
    marginTop: 12,
    textAlign: 'center',
    lineHeight: 16,
  },
});

export default SettingsScreen;