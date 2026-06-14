/**
 * PHANTOM — Device Configuration Screen
 * WiFi, BLE, stealth mode, geofencing, and hardware settings
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Switch,
  TextInput,
  Alert,
  Slider,
} from 'react-native';
import { BleContext } from '../components/BleContext';

const SettingsScreen = () => {
  const { connectedDevice, sendCommand, deviceInfo } = useContext(BleContext);

  // Device settings
  const [stealthMode, setStealthMode] = useState(true);
  const [autoHid, setAutoHid] = useState(false);
  const [autoHidDelay, setAutoHidDelay] = useState(5);
  const [defaultProfile, setDefaultProfile] = useState(0);
  const [oledBrightness, setOledBrightness] = useState(200);
  const [ledBrightness, setLedBrightness] = useState(50);
  const [killSwitchEnabled, setKillSwitchEnabled] = useState(true);
  const [geofenceEnabled, setGeofenceEnabled] = useState(false);
  const [geofenceMode, setGeofenceMode] = useState('whitelist');
  const [deviceName, setDeviceName] = useState('PHANTOM-01');
  const [defaultDelay, setDefaultDelay] = useState('5');

  // WiFi settings
  const [wifiMode, setWifiMode] = useState('station');
  const [wifiSsid, setWifiSsid] = useState('');
  const [wifiPassword, setWifiPassword] = useState('');
  const [wifiConnected, setWifiConnected] = useState(false);
  const [wifiApSsid, setWifiApSsid] = useState('PHANTOM-AP');
  const [wifiApPassword, setWifiApPassword] = useState('');

  // Geofence networks
  const [geofenceNetworks, setGeofenceNetworks] = useState([]);

  // About info
  const [firmwareVersion, setFirmwareVersion] = useState('1.0.0');
  const [hardwareRevision, setHardwareRevision] = useState('A1');
  const [serialNumber, setSerialNumber] = useState('');

  useEffect(() => {
    if (connectedDevice) {
      loadSettings();
    }
  }, [connectedDevice]);

  const loadSettings = async () => {
    try {
      const settings = await sendCommand('GET_SETTINGS');
      if (settings) {
        setStealthMode(settings.stealthMode ?? true);
        setAutoHid(settings.autoHid ?? false);
        setAutoHidDelay(settings.autoHidDelay ?? 5);
        setDefaultProfile(settings.defaultProfile ?? 0);
        setOledBrightness(settings.oledBrightness ?? 200);
        setLedBrightness(settings.ledBrightness ?? 50);
        setDeviceName(settings.deviceName ?? 'PHANTOM-01');
        setDefaultDelay(String(settings.defaultDelay ?? 5));
        setGeofenceEnabled(settings.geofenceEnabled ?? false);
        setGeofenceMode(settings.geofenceMode ?? 'whitelist');
      }
    } catch (error) {
      // Use defaults if device not reachable
    }
  };

  const saveSettings = async () => {
    if (!connectedDevice) {
      Alert.alert('Not Connected', 'Connect to device to save settings');
      return;
    }

    try {
      await sendCommand('SET_SETTINGS', {
        stealthMode,
        autoHid,
        autoHidDelay,
        defaultProfile,
        oledBrightness,
        ledBrightness,
        deviceName,
        defaultDelay: parseInt(defaultDelay) || 5,
        geofenceEnabled,
        geofenceMode,
      });
      Alert.alert('Saved', 'Settings saved to device');
    } catch (error) {
      Alert.alert('Error', `Failed to save: ${error.message}`);
    }
  };

  const handleFactoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will erase all profiles and settings. Are you sure?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: async () => {
            try {
              await sendCommand('FACTORY_RESET');
              Alert.alert('Reset Complete', 'Device has been factory reset');
              loadSettings();
            } catch (error) {
              Alert.alert('Error', error.message);
            }
          },
        },
      ]
    );
  };

  const handleFirmwareUpdate = () => {
    Alert.alert(
      'Firmware Update',
      'Check for firmware updates?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Check',
          onPress: async () => {
            try {
              const result = await sendCommand('CHECK_UPDATE');
              if (result.updateAvailable) {
                Alert.alert(
                  'Update Available',
                  `Version ${result.version} available. Update now?`,
                  [
                    { text: 'Later', style: 'cancel' },
                    {
                      text: 'Update',
                      onPress: async () => {
                        await sendCommand('UPDATE_FIRMWARE');
                      },
                    },
                  ]
                );
              } else {
                Alert.alert('Up to Date', 'Firmware is current');
              }
            } catch (error) {
              Alert.alert('Error', error.message);
            }
          },
        },
      ]
    );
  };

  const handleWiFiConnect = async () => {
    if (!connectedDevice) return;
    try {
      await sendCommand(`WIFI_CONNECT:${wifiSsid}:${wifiPassword}`);
      setWifiConnected(true);
      Alert.alert('Connected', `Connected to ${wifiSsid}`);
    } catch (error) {
      Alert.alert('WiFi Error', error.message);
    }
  };

  const handleWiFiDisconnect = async () => {
    if (!connectedDevice) return;
    try {
      await sendCommand('WIFI_DISCONNECT');
      setWifiConnected(false);
    } catch (error) {
      Alert.alert('Error', error.message);
    }
  };

  const handleGeofenceScan = async () => {
    if (!connectedDevice) return;
    try {
      const result = await sendCommand('WIFI_SCAN');
      if (result && result.networks) {
        setGeofenceNetworks(result.networks);
      }
    } catch (error) {
      Alert.alert('Scan Error', error.message);
    }
  };

  const SettingSection = ({ title, children }) => (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>{title}</Text>
      {children}
    </View>
  );

  const SettingRow = ({ label, children }) => (
    <View style={styles.settingRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      <View style={styles.settingControl}>{children}</View>
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      {/* USB Mode Settings */}
      <SettingSection title="USB MODE">
        <SettingRow label="Stealth Mode (MSC)">
          <Switch
            value={stealthMode}
            onValueChange={setStealthMode}
            trackColor={{ false: '#2d2d44', true: '#00ff41' }}
            thumbColor={stealthMode ? '#0f0f23' : '#666'}
          />
        </SettingRow>
        <SettingRow label="Auto-switch to HID">
          <Switch
            value={autoHid}
            onValueChange={setAutoHid}
            trackColor={{ false: '#2d2d44', true: '#00ff41' }}
            thumbColor={autoHid ? '#0f0f23' : '#666'}
          />
        </SettingRow>
        {autoHid && (
          <SettingRow label="HID Delay (seconds)">
            <TextInput
              style={styles.numberInput}
              value={String(autoHidDelay)}
              onChangeText={(v) => setAutoHidDelay(parseInt(v) || 0)}
              keyboardType="numeric"
            />
          </SettingRow>
        )}
        <SettingRow label="Default Key Delay (ms)">
          <TextInput
            style={styles.numberInput}
            value={defaultDelay}
            onChangeText={setDefaultDelay}
            keyboardType="numeric"
          />
        </SettingRow>
        <SettingRow label="Default Profile">
          <TextInput
            style={styles.numberInput}
            value={String(defaultProfile)}
            onChangeText={(v) => setDefaultProfile(parseInt(v) || 0)}
            keyboardType="numeric"
          />
        </SettingRow>
      </SettingSection>

      {/* Display Settings */}
      <SettingSection title="DISPLAY">
        <SettingRow label="OLED Brightness">
          <Slider
            style={styles.slider}
            minimumValue={0}
            maximumValue={255}
            value={oledBrightness}
            onValueChange={setOledBrightness}
            minimumTrackTintColor="#00ff41"
            maximumTrackTintColor="#2d2d44"
            thumbTintColor="#fff"
          />
        </SettingRow>
        <SettingRow label="LED Brightness">
          <Slider
            style={styles.slider}
            minimumValue={0}
            maximumValue={255}
            value={ledBrightness}
            onValueChange={setLedBrightness}
            minimumTrackTintColor="#00ff41"
            maximumTrackTintColor="#2d2d44"
            thumbTintColor="#fff"
          />
        </SettingRow>
      </SettingSection>

      {/* Security Settings */}
      <SettingSection title="SECURITY">
        <SettingRow label="Kill Switch">
          <Switch
            value={killSwitchEnabled}
            onValueChange={setKillSwitchEnabled}
            trackColor={{ false: '#2d2d44', true: '#00ff41' }}
            thumbColor={killSwitchEnabled ? '#0f0f23' : '#666'}
          />
        </SettingRow>
        <SettingRow label="Geofencing">
          <Switch
            value={geofenceEnabled}
            onValueChange={setGeofenceEnabled}
            trackColor={{ false: '#2d2d44', true: '#00ff41' }}
            thumbColor={geofenceEnabled ? '#0f0f23' : '#666'}
          />
        </SettingRow>
        {geofenceEnabled && (
          <>
            <View style={styles.radioGroup}>
              <TouchableOpacity
                style={[
                  styles.radioButton,
                  geofenceMode === 'whitelist' && styles.radioButtonActive,
                ]}
                onPress={() => setGeofenceMode('whitelist')}
              >
                <Text style={styles.radioText}>Whitelist</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[
                  styles.radioButton,
                  geofenceMode === 'blacklist' && styles.radioButtonActive,
                ]}
                onPress={() => setGeofenceMode('blacklist')}
              >
                <Text style={styles.radioText}>Blacklist</Text>
              </TouchableOpacity>
            </View>
            <TouchableOpacity
              style={styles.scanButton}
              onPress={handleGeofenceScan}
            >
              <Text style={styles.scanButtonText}>📡 Scan Networks</Text>
            </TouchableOpacity>
          </>
        )}
      </SettingSection>

      {/* WiFi Settings */}
      <SettingSection title="WIFI">
        <View style={styles.radioGroup}>
          <TouchableOpacity
            style={[
              styles.radioButton,
              wifiMode === 'station' && styles.radioButtonActive,
            ]}
            onPress={() => setWifiMode('station')}
          >
            <Text style={styles.radioText}>Station</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[
              styles.radioButton,
              wifiMode === 'ap' && styles.radioButtonActive,
            ]}
            onPress={() => setWifiMode('ap')}
          >
            <Text style={styles.radioText}>AP</Text>
          </TouchableOpacity>
        </View>

        {wifiMode === 'station' ? (
          <>
            <SettingRow label="SSID">
              <TextInput
                style={styles.textInput}
                value={wifiSsid}
                onChangeText={setWifiSsid}
                placeholder="Network name"
                placeholderTextColor="#555"
                autoCapitalize="none"
              />
            </SettingRow>
            <SettingRow label="Password">
              <TextInput
                style={styles.textInput}
                value={wifiPassword}
                onChangeText={setWifiPassword}
                placeholder="Password"
                placeholderTextColor="#555"
                secureTextEntry
                autoCapitalize="none"
              />
            </SettingRow>
            <TouchableOpacity
              style={styles.wifiButton}
              onPress={wifiConnected ? handleWiFiDisconnect : handleWiFiConnect}
            >
              <Text style={styles.wifiButtonText}>
                {wifiConnected ? 'Disconnect' : 'Connect'}
              </Text>
            </TouchableOpacity>
          </>
        ) : (
          <>
            <SettingRow label="AP SSID">
              <TextInput
                style={styles.textInput}
                value={wifiApSsid}
                onChangeText={setWifiApSsid}
                placeholder="AP name"
                placeholderTextColor="#555"
              />
            </SettingRow>
            <SettingRow label="AP Password">
              <TextInput
                style={styles.textInput}
                value={wifiApPassword}
                onChangeText={setWifiApPassword}
                placeholder="AP password (min 8 chars)"
                placeholderTextColor="#555"
                secureTextEntry
              />
            </SettingRow>
          </>
        )}
      </SettingSection>

      {/* BLE Settings */}
      <SettingSection title="BLE">
        <SettingRow label="Device Name">
          <TextInput
            style={styles.textInput}
            value={deviceName}
            onChangeText={setDeviceName}
            placeholder="BLE device name"
            placeholderTextColor="#555"
            autoCapitalize="none"
          />
        </SettingRow>
      </SettingSection>

      {/* Actions */}
      <SettingSection title="ACTIONS">
        <TouchableOpacity style={styles.actionButton} onPress={saveSettings}>
          <Text style={styles.actionButtonText}>💾 Save Settings</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, styles.updateButton]}
          onPress={handleFirmwareUpdate}
        >
          <Text style={styles.actionButtonText}>🔄 Check Firmware Update</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, styles.dangerButton]}
          onPress={handleFactoryReset}
        >
          <Text style={styles.actionButtonText}>⚠️ Factory Reset</Text>
        </TouchableOpacity>
      </SettingSection>

      {/* About */}
      <SettingSection title="ABOUT">
        <SettingRow label="Firmware">
          <Text style={styles.settingValue}>v{firmwareVersion}</Text>
        </SettingRow>
        <SettingRow label="Hardware">
          <Text style={styles.settingValue}>Rev {hardwareRevision}</Text>
        </SettingRow>
        <SettingRow label="Serial">
          <Text style={styles.settingValue}>
            {serialNumber || deviceInfo?.serial || 'N/A'}
          </Text>
        </SettingRow>
        <SettingRow label="Profiles">
          <Text style={styles.settingValue}>
            {deviceInfo?.profileCount || 0} loaded
          </Text>
        </SettingRow>
      </SettingSection>

      {/* Disclaimer */}
      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerTitle}>⚠️ DISCLAIMER</Text>
        <Text style={styles.disclaimerText}>
          This tool is designed for authorized security research and penetration
          testing only. The user is solely responsible for ensuring compliance
          with all applicable laws and regulations. Unauthorized access to
          computer systems is a criminal offense in most jurisdictions.
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
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  sectionTitle: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 12,
    fontWeight: 'bold',
    letterSpacing: 2,
    marginBottom: 12,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1a1a2e',
  },
  settingLabel: {
    color: '#ccc',
    fontFamily: 'monospace',
    fontSize: 13,
    flex: 1,
  },
  settingControl: {
    flex: 1,
    alignItems: 'flex-end',
  },
  settingValue: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 12,
  },
  numberInput: {
    backgroundColor: '#0f0f23',
    borderRadius: 6,
    padding: 8,
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 13,
    width: 80,
    textAlign: 'center',
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  textInput: {
    backgroundColor: '#0f0f23',
    borderRadius: 6,
    padding: 8,
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 13,
    width: 180,
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  slider: {
    width: 160,
    height: 30,
  },
  radioGroup: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  radioButton: {
    flex: 1,
    paddingVertical: 8,
    paddingHorizontal: 12,
    backgroundColor: '#0f0f23',
    borderRadius: 6,
    marginRight: 8,
    borderWidth: 1,
    borderColor: '#2d2d44',
    alignItems: 'center',
  },
  radioButtonActive: {
    backgroundColor: '#00ff41',
    borderColor: '#00ff41',
  },
  radioText: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 12,
  },
  scanButton: {
    backgroundColor: '#1a2d3d',
    borderRadius: 8,
    padding: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#3498db',
  },
  scanButtonText: {
    color: '#3498db',
    fontFamily: 'monospace',
    fontSize: 12,
  },
  wifiButton: {
    backgroundColor: '#00ff41',
    borderRadius: 8,
    padding: 10,
    alignItems: 'center',
    marginTop: 8,
  },
  wifiButtonText: {
    color: '#0f0f23',
    fontFamily: 'monospace',
    fontSize: 13,
    fontWeight: 'bold',
  },
  actionButton: {
    backgroundColor: '#2d2d44',
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#3d3d54',
  },
  updateButton: {
    borderColor: '#3498db',
  },
  dangerButton: {
    borderColor: '#e74c3c',
  },
  actionButtonText: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 13,
  },
  disclaimer: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 32,
    borderWidth: 1,
    borderColor: '#f39c12',
  },
  disclaimerTitle: {
    color: '#f39c12',
    fontFamily: 'monospace',
    fontSize: 11,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  disclaimerText: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 10,
    lineHeight: 16,
  },
});

export default SettingsScreen;