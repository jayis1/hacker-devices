/**
 * SettingsScreen.js — Device configuration and firmware management
 * Author: jayis1
 */

import React, {useState} from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  TextInput,
  Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const SettingsScreen = ({protocol, status}) => {
  const [wifiSSID, setWifiSSID] = useState('');
  const [wifiPass, setWifiPass] = useState('');
  const [brightness, setBrightness] = useState('128');
  const [contrast, setContrast] = useState('128');

  const handleApplyWifi = () => {
    if (!protocol) return;
    if (!wifiSSID.trim()) {
      Alert.alert('Error', 'SSID is required');
      return;
    }
    Alert.alert('WiFi Config', `Connecting to ${wifiSSID}...`);
    // In production, this would send WiFi credentials to device
  };

  const handleVideoConfig = () => {
    if (!protocol) return;
    protocol.sendCommand('config', {
      brightness: parseInt(brightness) || 128,
      contrast: parseInt(contrast) || 128,
    });
    Alert.alert('Applied', 'Video settings updated');
  };

  const handleFactoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will erase all settings. Continue?',
      [
        {text: 'Cancel', style: 'cancel'},
        {
          text: 'Reset',
          style: 'destructive',
          onPress: () => {
            if (protocol) {
              protocol.sendCommand('config', {factory_reset: true});
            }
          },
        },
      ],
    );
  };

  const deviceVersion = status?.version || '--';
  const fwVersion = status?.version || '--';

  return (
    <ScrollView style={styles.container}>
      {/* Device Info */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Information</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Device</Text>
          <Text style={styles.infoValue}>HDMI-Siphon</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Firmware</Text>
          <Text style={styles.infoValue}>v{fwVersion}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Author</Text>
          <Text style={styles.infoValue}>jayis1</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>FPGA</Text>
          <Text style={styles.infoValue}>
            {status?.fpga_ready ? 'Configured' : 'Not Ready'}
          </Text>
        </View>
      </View>

      {/* WiFi Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>WiFi Configuration</Text>
        <Text style={styles.inputLabel}>SSID</Text>
        <TextInput
          style={styles.textInput}
          value={wifiSSID}
          onChangeText={setWifiSSID}
          placeholder="Network name..."
          placeholderTextColor="#555"
        />
        <Text style={styles.inputLabel}>Password</Text>
        <TextInput
          style={styles.textInput}
          value={wifiPass}
          onChangeText={setWifiPass}
          placeholder="Password..."
          placeholderTextColor="#555"
          secureTextEntry
        />
        <TouchableOpacity style={styles.applyBtn} onPress={handleApplyWifi}>
          <Icon name="wifi" size={20} color="#FFF" />
          <Text style={styles.applyBtnText}>Apply WiFi</Text>
        </TouchableOpacity>
      </View>

      {/* Video Processing */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Video Processing</Text>
        <View style={styles.configRow}>
          <Text style={styles.configLabel}>Brightness (0-255)</Text>
          <TextInput
            style={styles.configInput}
            value={brightness}
            onChangeText={setBrightness}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.configRow}>
          <Text style={styles.configLabel}>Contrast (0-255)</Text>
          <TextInput
            style={styles.configInput}
            value={contrast}
            onChangeText={setContrast}
            keyboardType="numeric"
          />
        </View>
        <TouchableOpacity style={styles.applyBtn} onPress={handleVideoConfig}>
          <Icon name="eye" size={20} color="#FFF" />
          <Text style={styles.applyBtnText}>Apply Video Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Mode Shortcuts */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Mode Switch</Text>
        <View style={styles.modeRow}>
          {['passthrough', 'invert', 'grayscale', 'blank'].map(mode => (
            <TouchableOpacity
              key={mode}
              style={styles.modeBtn}
              onPress={() => protocol?.setMode(mode)}>
              <Text style={styles.modeBtnText}>
                {mode.charAt(0).toUpperCase() + mode.slice(1)}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Danger Zone */}
      <View style={[styles.card, styles.dangerCard]}>
        <Text style={[styles.cardTitle, styles.dangerTitle]}>Danger Zone</Text>
        <TouchableOpacity
          style={styles.dangerBtn}
          onPress={handleFactoryReset}>
          <Icon name="alert-octagon" size={20} color="#FFF" />
          <Text style={styles.dangerBtnText}>Factory Reset</Text>
        </TouchableOpacity>
      </View>

      {/* Footer */}
      <View style={styles.footer}>
        <Text style={styles.footerText}>HDMI-Siphon v1.0.0</Text>
        <Text style={styles.footerAuthor}>Author: jayis1</Text>
        <Text style={styles.footerLegal}>
          Authorized Security Research Only
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#121212',
    padding: 12,
  },
  card: {
    backgroundColor: '#1E1E2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: {
    color: '#FFF',
    fontSize: 18,
    fontWeight: '700',
    marginBottom: 12,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#2A2A3E',
  },
  infoLabel: {
    color: '#B0B0B0',
    fontSize: 14,
  },
  infoValue: {
    color: '#FFF',
    fontSize: 14,
    fontWeight: '600',
  },
  inputLabel: {
    color: '#B0B0B0',
    fontSize: 13,
    marginBottom: 4,
    marginTop: 8,
  },
  textInput: {
    backgroundColor: '#2A2A3E',
    color: '#FFF',
    borderRadius: 8,
    padding: 10,
    fontSize: 14,
  },
  configRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  configLabel: {
    color: '#B0B0B0',
    fontSize: 14,
  },
  configInput: {
    backgroundColor: '#2A2A3E',
    color: '#FFF',
    borderRadius: 6,
    padding: 8,
    width: 70,
    textAlign: 'center',
  },
  applyBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#E53935',
    padding: 12,
    borderRadius: 8,
    marginTop: 12,
  },
  applyBtnText: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '600',
    marginLeft: 8,
  },
  modeRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  modeBtn: {
    flex: 1,
    backgroundColor: '#2A2A3E',
    padding: 10,
    borderRadius: 8,
    alignItems: 'center',
    marginHorizontal: 3,
  },
  modeBtnText: {
    color: '#FFF',
    fontSize: 12,
    fontWeight: '600',
  },
  dangerCard: {
    borderWidth: 1,
    borderColor: '#C62828',
  },
  dangerTitle: {
    color: '#EF5350',
  },
  dangerBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#C62828',
    padding: 12,
    borderRadius: 8,
  },
  dangerBtnText: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '600',
    marginLeft: 8,
  },
  footer: {
    alignItems: 'center',
    paddingVertical: 20,
  },
  footerText: {
    color: '#78909C',
    fontSize: 13,
  },
  footerAuthor: {
    color: '#78909C',
    fontSize: 11,
    marginTop: 2,
  },
  footerLegal: {
    color: '#EF5350',
    fontSize: 10,
    marginTop: 4,
  },
});

export default SettingsScreen;
