/**
 * screens/SettingsScreen.js — Device Configuration Screen
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Switch, ScrollView, Alert } from 'react-native';
import { useDevice } from '../utils/deviceContext';
import { CMD, FMT, FMT_NAMES, MODE_NAMES, buildSetModePayload } from '../utils/protocol';

const SettingsScreen = () => {
  const { connected, sendCommand, firmwareVersion, batteryPct, batteryMv, uptime, fpgaReady } = useDevice();

  const [csiLanes, setCsiLanes] = useState('2');
  const [csiSpeed, setCsiSpeed] = useState('1000');
  const [csiWidth, setCsiWidth] = useState('1920');
  const [csiHeight, setCsiHeight] = useState('1080');
  const [csiFormat, setCsiFormat] = useState(FMT.RAW10);

  const [dsiLanes, setDsiLanes] = useState('2');
  const [dsiSpeed, setDsiSpeed] = useState('800');
  const [dsiWidth, setDsiWidth] = useState('1080');
  const [dsiHeight, setDsiHeight] = useState('1920');
  const [dsiFormat, setDsiFormat] = useState(FMT.RGB888);

  const [jpegCompress, setJpegCompress] = useState(false);
  const [selectedMode, setSelectedMode] = useState(1);

  // Load current config from device
  const loadConfig = async () => {
    if (!connected) return;
    await sendCommand(CMD.GET_CONFIG);
    Alert.alert('Config', 'Configuration loaded from device');
  };

  // Apply config to device
  const applyConfig = async () => {
    if (!connected) return;

    // Build config packet (must match firmware config_packet_t)
    const config = [
      parseInt(csiLanes) & 0xFF,
      parseInt(csiSpeed) & 0xFF,
      (parseInt(csiSpeed) >> 8) & 0xFF,
      (parseInt(csiSpeed) >> 16) & 0xFF,
      (parseInt(csiSpeed) >> 24) & 0xFF,
      parseInt(csiWidth) & 0xFF,
      (parseInt(csiWidth) >> 8) & 0xFF,
      parseInt(csiHeight) & 0xFF,
      (parseInt(csiHeight) >> 8) & 0xFF,
      csiFormat & 0xFF,
      parseInt(dsiLanes) & 0xFF,
      parseInt(dsiSpeed) & 0xFF,
      (parseInt(dsiSpeed) >> 8) & 0xFF,
      (parseInt(dsiSpeed) >> 16) & 0xFF,
      (parseInt(dsiSpeed) >> 24) & 0xFF,
      parseInt(dsiWidth) & 0xFF,
      (parseInt(dsiWidth) >> 8) & 0xFF,
      parseInt(dsiHeight) & 0xFF,
      (parseInt(dsiHeight) >> 8) & 0xFF,
      dsiFormat & 0xFF,
      jpegCompress ? 1 : 0,
      0, 0, 0, 0, 0, 0, 0, 0,  // reserved
    ];

    await sendCommand(CMD.SET_CONFIG, config);
    Alert.alert('Config', 'Configuration applied to device');
  };

  const applyMode = async (mode) => {
    setSelectedMode(mode);
    await sendCommand(CMD.SET_MODE, buildSetModePayload(mode));
    Alert.alert('Mode', `Set to ${MODE_NAMES[mode] || 'unknown'}`);
  };

  const formatUptime = (ms) => {
    if (!ms) return '---';
    const seconds = Math.floor(ms / 1000);
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h}h ${m}m ${s}s`;
  };

  return (
    <ScrollView style={styles.container}>
      {/* Device Info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Info</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Firmware:</Text>
          <Text style={styles.infoValue}>{firmwareVersion || '---'}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Battery:</Text>
          <Text style={styles.infoValue}>{batteryPct}% ({batteryMv} mV)</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Uptime:</Text>
          <Text style={styles.infoValue}>{formatUptime(uptime)}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>FPGA:</Text>
          <Text style={[styles.infoValue, { color: fpgaReady ? '#4caf50' : '#ff4444' }]}>
            {fpgaReady ? 'Ready' : 'Not Ready'}
          </Text>
        </View>
      </View>

      {/* Operating Mode */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Operating Mode</Text>
        {Object.entries(MODE_NAMES).map(([key, name]) => (
          <TouchableOpacity
            key={key}
            style={[styles.modeRow, selectedMode === parseInt(key) && styles.modeRowActive]}
            onPress={() => applyMode(parseInt(key))}
          >
            <Text style={[styles.modeName, selectedMode === parseInt(key) && styles.modeNameActive]}>
              {name}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* CSI-2 Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CSI-2 (Camera) Configuration</Text>
        <View style={styles.row}>
          <Text style={styles.label}>Lanes:</Text>
          <TextInput style={styles.input} value={csiLanes} onChangeText={setCsiLanes} keyboardType="numeric" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Speed (Mbps):</Text>
          <TextInput style={styles.input} value={csiSpeed} onChangeText={setCsiSpeed} keyboardType="numeric" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Width:</Text>
          <TextInput style={styles.input} value={csiWidth} onChangeText={setCsiWidth} keyboardType="numeric" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Height:</Text>
          <TextInput style={styles.input} value={csiHeight} onChangeText={setCsiHeight} keyboardType="numeric" />
        </View>
        <Text style={styles.label}>Format:</Text>
        <View style={styles.formatButtons}>
          {Object.entries(FMT_NAMES).map(([key, name]) => (
            <TouchableOpacity
              key={key}
              style={[styles.formatButton, csiFormat === parseInt(key) && styles.formatButtonActive]}
              onPress={() => setCsiFormat(parseInt(key))}
            >
              <Text style={[styles.formatText, csiFormat === parseInt(key) && styles.formatTextActive]}>
                {name}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* DSI Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>DSI (Display) Configuration</Text>
        <View style={styles.row}>
          <Text style={styles.label}>Lanes:</Text>
          <TextInput style={styles.input} value={dsiLanes} onChangeText={setDsiLanes} keyboardType="numeric" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Speed (Mbps):</Text>
          <TextInput style={styles.input} value={dsiSpeed} onChangeText={setDsiSpeed} keyboardType="numeric" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Width:</Text>
          <TextInput style={styles.input} value={dsiWidth} onChangeText={setDsiWidth} keyboardType="numeric" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Height:</Text>
          <TextInput style={styles.input} value={dsiHeight} onChangeText={setDsiHeight} keyboardType="numeric" />
        </View>
        <Text style={styles.label}>Format:</Text>
        <View style={styles.formatButtons}>
          {Object.entries(FMT_NAMES).map(([key, name]) => (
            <TouchableOpacity
              key={`dsi-${key}`}
              style={[styles.formatButton, dsiFormat === parseInt(key) && styles.formatButtonActive]}
              onPress={() => setDsiFormat(parseInt(key))}
            >
              <Text style={[styles.formatText, dsiFormat === parseInt(key) && styles.formatTextActive]}>
                {name}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Other Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Other</Text>
        <View style={styles.row}>
          <Text style={styles.label}>JPEG Compress:</Text>
          <Switch
            value={jpegCompress}
            onValueChange={setJpegCompress}
            trackColor={{ false: '#333', true: '#00d9ff' }}
          />
        </View>
      </View>

      {/* Action Buttons */}
      <View style={styles.actionSection}>
        <TouchableOpacity
          style={[styles.button, styles.loadButton, !connected && styles.disabled]}
          onPress={loadConfig}
          disabled={!connected}
        >
          <Text style={styles.buttonText}>Load from Device</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.button, styles.applyButton, !connected && styles.disabled]}
          onPress={applyConfig}
          disabled={!connected}
        >
          <Text style={styles.applyButtonText}>Apply Configuration</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.aboutBox}>
        <Text style={styles.aboutTitle}>Prism-Tap v1.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>License: GPL-2.0 / CERN-OHL-S v2</Text>
        <Text style={styles.aboutDisclaimer}>
          This device is for authorized security research and penetration
          testing only. Unauthorized interception of camera or display data
          may be illegal in your jurisdiction. Always obtain proper written
          authorization before deployment.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1e',
  },
  section: {
    padding: 15,
    borderBottomWidth: 1,
    borderBottomColor: '#1a1a2e',
  },
  sectionTitle: {
    color: '#00d9ff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 10,
  },
  infoRow: {
    flexDirection: 'row',
    marginBottom: 6,
  },
  infoLabel: {
    color: '#666',
    fontSize: 13,
    width: 100,
  },
  infoValue: {
    color: '#fff',
    fontSize: 13,
  },
  modeRow: {
    padding: 8,
    marginBottom: 4,
    borderRadius: 5,
    borderWidth: 1,
    borderColor: '#333',
    backgroundColor: '#1a1a2e',
  },
  modeRowActive: {
    borderColor: '#00d9ff',
    backgroundColor: '#0a2e3e',
  },
  modeName: {
    color: '#888',
    fontSize: 13,
  },
  modeNameActive: {
    color: '#00d9ff',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  label: {
    color: '#aaa',
    fontSize: 13,
    width: 120,
    marginBottom: 4,
  },
  input: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    color: '#fff',
    borderWidth: 1,
    borderColor: '#333',
    borderRadius: 5,
    paddingHorizontal: 8,
    paddingVertical: 4,
    fontSize: 13,
  },
  formatButtons: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginTop: 4,
  },
  formatButton: {
    padding: 4,
    margin: 2,
    borderRadius: 4,
    borderWidth: 1,
    borderColor: '#333',
    backgroundColor: '#1a1a2e',
  },
  formatButtonActive: {
    borderColor: '#00d9ff',
    backgroundColor: '#0a2e3e',
  },
  formatText: {
    color: '#666',
    fontSize: 10,
  },
  formatTextActive: {
    color: '#00d9ff',
  },
  actionSection: {
    padding: 15,
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  button: {
    flex: 1,
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  loadButton: {
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#00d9ff',
  },
  applyButton: {
    backgroundColor: '#00d9ff',
  },
  disabled: {
    opacity: 0.4,
  },
  buttonText: {
    color: '#00d9ff',
    fontSize: 13,
    fontWeight: 'bold',
  },
  applyButtonText: {
    color: '#0f0f1e',
    fontSize: 13,
    fontWeight: 'bold',
  },
  aboutBox: {
    margin: 15,
    padding: 15,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  aboutTitle: {
    color: '#00d9ff',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  aboutText: {
    color: '#888',
    fontSize: 11,
    marginBottom: 2,
  },
  aboutDisclaimer: {
    color: '#553333',
    fontSize: 10,
    marginTop: 8,
    lineHeight: 14,
  },
});

export default SettingsScreen;