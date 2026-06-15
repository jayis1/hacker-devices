/**
 * SettingsScreen.js — App & Device Configuration
 *
 * Provides settings for the companion app and the eMMC Flash Dumper
 * hardware, including acquisition defaults, display preferences,
 * data export options, and firmware update capability.
 *
 * Copyright (c) 2026 Nous Research
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Switch,
  TextInput,
  Alert,
  ActivityIndicator,
} from 'react-native';
import { DeviceContext } from '../components/DeviceContext';

/*===========================================================================
 * SettingsScreen Component
 *===========================================================================*/

export default function SettingsScreen() {
  const device = useContext(DeviceContext);

  /* App Settings */
  const [darkMode, setDarkMode] = useState(true);
  const [hexUpperCase, setHexUpperCase] = useState(true);
  const [hexGroupSize, setHexGroupSize] = useState('1');
  const [autoVerifyHash, setAutoVerifyHash] = useState(true);
  const [autoSaveToSd, setAutoSaveToSd] = useState(false);
  const [progressUpdateMs, setProgressUpdateMs] = useState('100');

  /* Acquisition Defaults */
  const [defaultBlockSize, setDefaultBlockSize] = useState('65536');
  const [defaultRetryCount, setDefaultRetryCount] = useState('3');
  const [defaultTimeout, setDefaultTimeout] = useState('30');
  const [preferHs400, setPreferHs400] = useState(true);
  const [readBootPartitions, setReadBootPartitions] = useState(false);
  const [readRpmb, setReadRpmb] = useState(false);

  /* Export Settings */
  const [exportFormat, setExportFormat] = useState('raw');
  const [exportCompress, setExportCompress] = useState(false);
  const [exportSplitSize, setExportSplitSize] = useState('0');

  /* Firmware Update */
  const [fwUpdateAvailable, setFwUpdateAvailable] = useState(false);
  const [fwUpdating, setFwUpdating] = useState(false);

  /*=======================================================================
   * Save Settings
   *=======================================================================*/

  const saveSettings = useCallback(() => {
    /* In a real app, persist to AsyncStorage */
    Alert.alert('Settings Saved', 'Configuration has been saved.');
  }, []);

  /*=======================================================================
   * Reset to Defaults
   *=======================================================================*/

  const resetDefaults = useCallback(() => {
    Alert.alert(
      'Reset Settings',
      'Restore all settings to factory defaults?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: () => {
            setDarkMode(true);
            setHexUpperCase(true);
            setHexGroupSize('1');
            setAutoVerifyHash(true);
            setAutoSaveToSd(false);
            setProgressUpdateMs('100');
            setDefaultBlockSize('65536');
            setDefaultRetryCount('3');
            setDefaultTimeout('30');
            setPreferHs400(true);
            setReadBootPartitions(false);
            setReadRpmb(false);
            setExportFormat('raw');
            setExportCompress(false);
            setExportSplitSize('0');
          },
        },
      ]
    );
  }, []);

  /*=======================================================================
   * Firmware Update
   *=======================================================================*/

  const checkFirmwareUpdate = useCallback(async () => {
    /* In a real app, check against a server */
    setFwUpdateAvailable(false);
    Alert.alert('Firmware', 'Device firmware is up to date (v1.0.0).');
  }, []);

  const startFirmwareUpdate = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Connect device to update firmware.');
      return;
    }

    Alert.alert(
      'Firmware Update',
      'This will update the device firmware. The device will reboot after update. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Update',
          style: 'destructive',
          onPress: async () => {
            setFwUpdating(true);
            try {
              /* Firmware update via USB DFU or custom protocol */
              await new Promise(r => setTimeout(r, 3000));  /* Simulated */
              Alert.alert('Update Complete', 'Firmware updated successfully. Device rebooting...');
            } catch (e) {
              Alert.alert('Update Failed', e.message);
            } finally {
              setFwUpdating(false);
            }
          },
        },
      ]
    );
  }, [device.connected]);

  /*=======================================================================
   * Render
   *=======================================================================*/

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* App Settings */}
      <Text style={styles.sectionTitle}>APP SETTINGS</Text>
      <View style={styles.settingsCard}>
        <SwitchRow label="Dark Mode" value={darkMode} onChange={setDarkMode} />
        <SwitchRow label="Hex Uppercase" value={hexUpperCase} onChange={setHexUpperCase} />
        <SwitchRow label="Auto-Verify Hash" value={autoVerifyHash} onChange={setAutoVerifyHash} />
        <SwitchRow label="Auto-Save to SD" value={autoSaveToSd} onChange={setAutoSaveToSd} />
        <InputRow
          label="Hex Group Size"
          value={hexGroupSize}
          onChange={setHexGroupSize}
          hint="1, 2, 4, 8"
        />
        <InputRow
          label="Progress Update (ms)"
          value={progressUpdateMs}
          onChange={setProgressUpdateMs}
          hint="50-1000"
        />
      </View>

      {/* Acquisition Defaults */}
      <Text style={styles.sectionTitle}>ACQUISITION DEFAULTS</Text>
      <View style={styles.settingsCard}>
        <SwitchRow label="Prefer HS400 Mode" value={preferHs400} onChange={setPreferHs400} />
        <SwitchRow label="Read Boot Partitions" value={readBootPartitions} onChange={setReadBootPartitions} />
        <SwitchRow label="Read RPMB (if key known)" value={readRpmb} onChange={setReadRpmb} />
        <InputRow
          label="DMA Block Size (bytes)"
          value={defaultBlockSize}
          onChange={setDefaultBlockSize}
          hint="4096-131072"
        />
        <InputRow
          label="Max Retries"
          value={defaultRetryCount}
          onChange={setDefaultRetryCount}
          hint="1-10"
        />
        <InputRow
          label="Timeout (seconds)"
          value={defaultTimeout}
          onChange={setDefaultTimeout}
          hint="5-120"
        />
      </View>

      {/* Export Settings */}
      <Text style={styles.sectionTitle}>EXPORT SETTINGS</Text>
      <View style={styles.settingsCard}>
        <View style={styles.optionRow}>
          <Text style={styles.optionLabel}>Format</Text>
          <View style={styles.segmentRow}>
            {['raw', 'img', 'bin'].map((fmt) => (
              <TouchableOpacity
                key={fmt}
                style={[styles.segment, exportFormat === fmt && styles.segmentActive]}
                onPress={() => setExportFormat(fmt)}
              >
                <Text style={[styles.segmentText, exportFormat === fmt && styles.segmentTextActive]}>
                  {fmt.toUpperCase()}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
        <SwitchRow label="Compress (gzip)" value={exportCompress} onChange={setExportCompress} />
        <InputRow
          label="Split Size (MB, 0=no split)"
          value={exportSplitSize}
          onChange={setExportSplitSize}
          hint="0-4096"
        />
      </View>

      {/* Firmware Update */}
      <Text style={styles.sectionTitle}>FIRMWARE</Text>
      <View style={styles.settingsCard}>
        <View style={styles.fwRow}>
          <View>
            <Text style={styles.fwLabel}>Current Version</Text>
            <Text style={styles.fwValue}>
              {device.deviceInfo?.fwVersion || '—'}
            </Text>
          </View>
          {fwUpdateAvailable && (
            <View style={styles.fwBadge}>
              <Text style={styles.fwBadgeText}>UPDATE AVAILABLE</Text>
            </View>
          )}
        </View>
        <View style={styles.fwActions}>
          <TouchableOpacity style={styles.fwCheckBtn} onPress={checkFirmwareUpdate}>
            <Text style={styles.fwCheckBtnText}>CHECK FOR UPDATE</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.fwUpdateBtn, (!fwUpdateAvailable || fwUpdating) && styles.btnDisabled]}
            onPress={startFirmwareUpdate}
            disabled={!fwUpdateAvailable || fwUpdating}
          >
            {fwUpdating ? (
              <ActivityIndicator color="#fff" size="small" />
            ) : (
              <Text style={styles.fwUpdateBtnText}>UPDATE FIRMWARE</Text>
            )}
          </TouchableOpacity>
        </View>
      </View>

      {/* About */}
      <Text style={styles.sectionTitle}>ABOUT</Text>
      <View style={styles.settingsCard}>
        <InfoRow label="App Version" value="1.0.0" />
        <InfoRow label="Protocol Version" value="1.0" />
        <InfoRow label="Build Date" value="2026-06-15" />
        <InfoRow label="License" value="MIT" />
        <InfoRow label="Author" value="Nous Research" />
      </View>

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.saveBtn} onPress={saveSettings}>
          <Text style={styles.saveBtnText}>SAVE SETTINGS</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.resetBtn} onPress={resetDefaults}>
          <Text style={styles.resetBtnText}>RESET DEFAULTS</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

/*===========================================================================
 * Sub-Components
 *===========================================================================*/

function SwitchRow({ label, value, onChange }) {
  return (
    <View style={styles.switchRow}>
      <Text style={styles.switchLabel}>{label}</Text>
      <Switch
        value={value}
        onValueChange={onChange}
        trackColor={{ false: '#333', true: '#00ff8840' }}
        thumbColor={value ? '#00ff88' : '#666'}
      />
    </View>
  );
}

function InputRow({ label, value, onChange, hint }) {
  return (
    <View style={styles.inputRow}>
      <View style={styles.inputLabelRow}>
        <Text style={styles.inputLabel}>{label}</Text>
        {hint && <Text style={styles.inputHint}>({hint})</Text>}
      </View>
      <TextInput
        style={styles.textInput}
        value={value}
        onChangeText={onChange}
        keyboardType="numeric"
      />
    </View>
  );
}

function InfoRow({ label, value }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

/*===========================================================================
 * Styles
 *===========================================================================*/

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  sectionTitle: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#666',
    letterSpacing: 2,
    marginBottom: 8,
    marginTop: 20,
  },
  settingsCard: {
    backgroundColor: '#111',
    borderRadius: 8,
    padding: 12,
    marginBottom: 4,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 0.5,
    borderBottomColor: '#1a1a1a',
  },
  switchLabel: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#ccc',
  },
  inputRow: {
    paddingVertical: 8,
    borderBottomWidth: 0.5,
    borderBottomColor: '#1a1a1a',
  },
  inputLabelRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 4,
  },
  inputLabel: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#ccc',
  },
  inputHint: {
    fontFamily: 'monospace',
    fontSize: 9,
    color: '#555',
    marginLeft: 6,
  },
  textInput: {
    backgroundColor: '#1a1a1a',
    borderRadius: 4,
    padding: 8,
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#fff',
    borderWidth: 1,
    borderColor: '#333',
  },
  optionRow: {
    paddingVertical: 8,
    borderBottomWidth: 0.5,
    borderBottomColor: '#1a1a1a',
  },
  optionLabel: {
    fontFamily: 'monospace',
    fontSize: 12,
    color: '#ccc',
    marginBottom: 6,
  },
  segmentRow: {
    flexDirection: 'row',
    gap: 4,
  },
  segment: {
    flex: 1,
    backgroundColor: '#1a1a1a',
    borderRadius: 4,
    paddingVertical: 6,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  segmentActive: {
    backgroundColor: '#1a2e1a',
    borderColor: '#00ff88',
  },
  segmentText: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#888',
  },
  segmentTextActive: {
    color: '#00ff88',
  },
  fwRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  fwLabel: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#666',
  },
  fwValue: {
    fontFamily: 'monospace',
    fontSize: 14,
    color: '#fff',
  },
  fwBadge: {
    backgroundColor: '#1a1a00',
    borderRadius: 4,
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderWidth: 1,
    borderColor: '#ff8800',
  },
  fwBadgeText: {
    fontFamily: 'monospace',
    fontSize: 8,
    color: '#ff8800',
  },
  fwActions: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 8,
  },
  fwCheckBtn: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    borderRadius: 6,
    padding: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  fwCheckBtnText: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#8888ff',
  },
  fwUpdateBtn: {
    flex: 1,
    backgroundColor: '#1a2e1a',
    borderRadius: 6,
    padding: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#00ff88',
  },
  fwUpdateBtnText: {
    fontFamily: 'monospace',
    fontSize: 10,
    color: '#00ff88',
  },
  btnDisabled: {
    opacity: 0.4,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 3,
  },
  infoLabel: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#888',
  },
  infoValue: {
    fontFamily: 'monospace',
    fontSize: 11,
    color: '#fff',
  },
  actionRow: {
    flexDirection: 'row',
    gap: 12,
    marginTop: 24,
  },
  saveBtn: {
    flex: 1,
    backgroundColor: '#003322',
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#00ff88',
  },
  saveBtnText: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#00ff88',
    letterSpacing: 2,
  },
  resetBtn: {
    flex: 1,
    backgroundColor: '#1a1a1a',
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#444',
  },
  resetBtnText: {
    fontFamily: 'monospace',
    fontSize: 13,
    color: '#888',
    letterSpacing: 2,
  },
});
