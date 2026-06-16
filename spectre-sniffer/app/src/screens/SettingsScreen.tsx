//==============================================================================
// SettingsScreen.tsx — Device Configuration View
// Author: jayis1
// Description: Settings and configuration management for the Spectre-Sniffer
//              device, including WiFi, calibration, and system settings.
//==============================================================================

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  TextInput,
  Switch,
  Alert,
} from 'react-native';
import { SpectreAPI } from '../services/api';
import { DeviceConfig } from '../types';

const SettingsScreen: React.FC = () => {
  const [config, setConfig] = useState<DeviceConfig | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadConfig();
  }, []);

  const loadConfig = async () => {
    try {
      const data = await SpectreAPI.getConfig();
      setConfig(data);
    } catch (err) {
      // Use defaults if device not connected
      setConfig({
        wifiApSsid: 'Spectre-Sniffer',
        wifiApPassword: 'spectre123',
        wifiChannel: 6,
        bleAdvertiseInterval: 100,
        displayBrightness: 200,
        autoPowerOffSeconds: 300,
        calibrationProfile: 'default',
        dateFormat: 'YYYY-MM-DD',
      });
    }
    setLoading(false);
  };

  const saveConfig = async () => {
    if (!config) return;
    try {
      await SpectreAPI.updateConfig(config);
      Alert.alert('Success', 'Configuration saved');
    } catch (err) {
      Alert.alert('Error', 'Failed to save configuration');
    }
  };

  const rebootDevice = () => {
    Alert.alert('Reboot Device', 'Are you sure?', [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Reboot',
        style: 'destructive',
        onPress: async () => {
          try {
            await SpectreAPI.reboot();
          } catch (err) {
            // Device will disconnect
          }
        },
      },
    ]);
  };

  if (loading || !config) {
    return (
      <View style={styles.loadingContainer}>
        <Text style={styles.loadingText}>Loading settings...</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      {/* WiFi Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>WiFi Configuration</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>AP SSID</Text>
          <TextInput
            style={styles.settingInput}
            value={config.wifiApSsid}
            onChangeText={(text) => setConfig({ ...config, wifiApSsid: text })}
            placeholderTextColor="#666"
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>AP Password</Text>
          <TextInput
            style={styles.settingInput}
            value={config.wifiApPassword}
            onChangeText={(text) => setConfig({ ...config, wifiApPassword: text })}
            secureTextEntry
            placeholderTextColor="#666"
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Channel</Text>
          <TouchableOpacity
            style={styles.settingValue}
            onPress={() =>
              setConfig({
                ...config,
                wifiChannel: ((config.wifiChannel % 11) + 1),
              })
            }
          >
            <Text style={styles.settingValueText}>{config.wifiChannel}</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* BLE Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Bluetooth Configuration</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Advertise Interval (ms)</Text>
          <TouchableOpacity
            style={styles.settingValue}
            onPress={() => {
              const intervals = [50, 100, 200, 500, 1000];
              const idx = intervals.indexOf(config.bleAdvertiseInterval);
              setConfig({
                ...config,
                bleAdvertiseInterval: intervals[(idx + 1) % intervals.length],
              });
            }}
          >
            <Text style={styles.settingValueText}>
              {config.bleAdvertiseInterval}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Display Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Brightness</Text>
          <TouchableOpacity
            style={styles.settingValue}
            onPress={() =>
              setConfig({
                ...config,
                displayBrightness:
                  config.displayBrightness >= 255 ? 50 : config.displayBrightness + 50,
              })
            }
          >
            <Text style={styles.settingValueText}>
              {config.displayBrightness}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Power Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Power Management</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto Power-Off (s)</Text>
          <TouchableOpacity
            style={styles.settingValue}
            onPress={() => {
              const timeouts = [0, 60, 300, 600, 1800, 3600];
              const idx = timeouts.indexOf(config.autoPowerOffSeconds);
              setConfig({
                ...config,
                autoPowerOffSeconds: timeouts[(idx + 1) % timeouts.length],
              });
            }}
          >
            <Text style={styles.settingValueText}>
              {config.autoPowerOffSeconds === 0
                ? 'Disabled'
                : `${config.autoPowerOffSeconds}s`}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Calibration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Calibration</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Profile</Text>
          <TouchableOpacity
            style={styles.settingValue}
            onPress={() => {
              const profiles = ['default', 'h-field', 'e-field', 'log-periodic'];
              const idx = profiles.indexOf(config.calibrationProfile);
              setConfig({
                ...config,
                calibrationProfile: profiles[(idx + 1) % profiles.length],
              });
            }}
          >
            <Text style={styles.settingValueText}>
              {config.calibrationProfile}
            </Text>
          </TouchableOpacity>
        </View>
        <TouchableOpacity
          style={styles.calibrateButton}
          onPress={async () => {
            try {
              await SpectreAPI.calibrateProbe(config.calibrationProfile);
              Alert.alert('Calibration', 'Probe calibration started');
            } catch (err) {
              Alert.alert('Error', 'Calibration failed');
            }
          }}
        >
          <Text style={styles.calibrateButtonText}>Run Calibration</Text>
        </TouchableOpacity>
      </View>

      {/* Action Buttons */}
      <View style={styles.actions}>
        <TouchableOpacity style={styles.saveButton} onPress={saveConfig}>
          <Text style={styles.saveButtonText}>Save Configuration</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.rebootButton} onPress={rebootDevice}>
          <Text style={styles.rebootButtonText}>Reboot Device</Text>
        </TouchableOpacity>
      </View>

      {/* Device Info */}
      <View style={styles.infoSection}>
        <Text style={styles.infoText}>Spectre-Sniffer v1.0</Text>
        <Text style={styles.infoText}>Author: jayis1</Text>
        <Text style={styles.warningText}>
          For authorized security research use only
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  loadingContainer: {
    flex: 1,
    backgroundColor: '#0f0f23',
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    color: '#888',
    fontSize: 16,
  },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  sectionTitle: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a4e',
  },
  settingLabel: {
    color: '#aaa',
    fontSize: 14,
    flex: 1,
  },
  settingInput: {
    backgroundColor: '#0f0f23',
    color: '#fff',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    fontSize: 14,
    flex: 1,
    textAlign: 'right',
  },
  settingValue: {
    backgroundColor: '#0f0f23',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
  },
  settingValueText: {
    color: '#00E676',
    fontSize: 14,
    fontWeight: '600',
  },
  calibrateButton: {
    backgroundColor: '#E040FB',
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 12,
  },
  calibrateButtonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  actions: {
    marginBottom: 24,
  },
  saveButton: {
    backgroundColor: '#00E676',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 12,
  },
  saveButtonText: {
    color: '#000',
    fontSize: 16,
    fontWeight: 'bold',
  },
  rebootButton: {
    backgroundColor: '#FF5252',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  rebootButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  infoSection: {
    alignItems: 'center',
    paddingVertical: 24,
  },
  infoText: {
    color: '#555',
    fontSize: 12,
    marginBottom: 4,
  },
  warningText: {
    color: '#FF5252',
    fontSize: 11,
    marginTop: 8,
    textAlign: 'center',
  },
});

export default SettingsScreen;
