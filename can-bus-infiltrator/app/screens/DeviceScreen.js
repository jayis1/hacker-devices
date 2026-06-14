/**
 * DeviceScreen.js — Device settings and status
 * BLE connection, firmware update, LED control, diagnostics
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Switch,
  Alert,
  TextInput,
} from 'react-native';
import { BLE_PROTOCOL } from '../utils/protocol';

export default function DeviceScreen() {
  const [connected, setConnected] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [can1Baud, setCan1Baud] = useState(2); // 500K
  const [can2Baud, setCan2Baud] = useState(2);
  const [recording, setRecording] = useState(false);
  const [replaying, setReplaying] = useState(false);
  const [ledColors, setLedColors] = useState([
    { r: 0, g: 255, b: 0 },
    { r: 0, g: 255, b: 0 },
    { r: 0, g: 255, b: 0 },
    { r: 0, g: 255, b: 0 },
  ]);

  const BAUD_LABELS = ['125K', '250K', '500K', '1M'];

  const connect = async () => {
    try {
      const result = await BLE_PROTOCOL.connect();
      if (result) {
        setConnected(true);
        await getDeviceInfo();
      } else {
        Alert.alert('Error', 'Could not connect to CAN Infiltrator');
      }
    } catch (e) {
      Alert.alert('Error', `Connection failed: ${e.message}`);
    }
  };

  const disconnect = async () => {
    await BLE_PROTOCOL.disconnect();
    setConnected(false);
    setDeviceInfo(null);
  };

  const getDeviceInfo = async () => {
    try {
      const response = await BLE_PROTOCOL.sendCommandWithResponse(0x0D, new Uint8Array(0));
      if (response && response.length >= 7) {
        setDeviceInfo({
          versionMajor: response[1],
          versionMinor: response[2],
          channels: response[3],
          features: response[4],
          buildVersion: (response[5] << 8) | response[6],
        });
      }
    } catch (e) {
      console.warn('Failed to get device info:', e);
    }
  };

  const toggleRecording = async () => {
    if (recording) {
      await BLE_PROTOCOL.sendCommand(0x09, new Uint8Array(0));
      setRecording(false);
    } else {
      await BLE_PROTOCOL.sendCommand(0x08, new Uint8Array(0));
      setRecording(true);
    }
  };

  const setLed = async (index, r, g, b) => {
    const cmd = new Uint8Array([index, r, g, b]);
    await BLE_PROTOCOL.sendCommand(0x0F, cmd);
    const newColors = [...ledColors];
    newColors[index] = { r, g, b };
    setLedColors(newColors);
  };

  const enterDFU = async () => {
    Alert.alert(
      'DFU Mode',
      'This will put the device into firmware update mode. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Enter DFU',
          style: 'destructive',
          onPress: async () => {
            await BLE_PROTOCOL.sendCommand(0x0E, new Uint8Array(0));
            setConnected(false);
            Alert.alert('DFU Mode', 'Device is now in firmware update mode.');
          },
        },
      ]
    );
  };

  const LED_PRESETS = [
    { name: 'Green (Ready)', colors: [{ r: 0, g: 255, b: 0 }, { r: 0, g: 255, b: 0 }, { r: 0, g: 255, b: 0 }, { r: 0, g: 255, b: 0 }] },
    { name: 'Red (Alert)', colors: [{ r: 255, g: 0, b: 0 }, { r: 255, g: 0, b: 0 }, { r: 255, g: 0, b: 0 }, { r: 255, g: 0, b: 0 }] },
    { name: 'Blue (Sniffing)', colors: [{ r: 0, g: 0, b: 255 }, { r: 0, g: 0, b: 255 }, { r: 0, g: 0, b: 255 }, { r: 0, g: 0, b: 255 }] },
    { name: 'Orange (Fuzzing)', colors: [{ r: 255, g: 165, b: 0 }, { r: 255, g: 165, b: 0 }, { r: 255, g: 165, b: 0 }, { r: 255, g: 165, b: 0 }] },
    { name: 'Off', colors: [{ r: 0, g: 0, b: 0 }, { r: 0, g: 0, b: 0 }, { r: 0, g: 0, b: 0 }, { r: 0, g: 0, b: 0 }] },
  ];

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Device Settings</Text>

      {/* Connection Status */}
      <View style={styles.section}>
        <View style={styles.statusRow}>
          <View style={[styles.statusDot, connected ? styles.dotGreen : styles.dotRed]} />
          <Text style={styles.statusText}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <TouchableOpacity
          style={[styles.connectButton, connected ? styles.disconnectStyle : styles.connectStyle]}
          onPress={connected ? disconnect : connect}
        >
          <Text style={styles.connectButtonText}>
            {connected ? 'Disconnect' : 'Connect via BLE'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Device Info */}
      {deviceInfo && (
        <View style={styles.section}>
          <Text style={styles.subtitle}>Device Information</Text>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Firmware:</Text>
            <Text style={styles.infoValue}>v{deviceInfo.versionMajor}.{deviceInfo.versionMinor}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Channels:</Text>
            <Text style={styles.infoValue}>{deviceInfo.channels}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Features:</Text>
            <Text style={styles.infoValue}>0x{deviceInfo.features.toString(16).toUpperCase()}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Build:</Text>
            <Text style={styles.infoValue}>0x{deviceInfo.buildVersion.toString(16).toUpperCase()}</Text>
          </View>
        </View>
      )}

      {/* CAN Baud Rate */}
      <View style={styles.section}>
        <Text style={styles.subtitle}>CAN Baud Rate</Text>
        <View style={styles.row}>
          <Text style={styles.label}>CH1:</Text>
          {BAUD_LABELS.map((label, idx) => (
            <TouchableOpacity
              key={`ch1-${idx}`}
              style={[styles.baudButton, can1Baud === idx ? styles.baudActive : null]}
              onPress={() => setCan1Baud(idx)}
            >
              <Text style={[styles.baudText, can1Baud === idx ? styles.baudTextActive : null]}>
                {label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>CH2:</Text>
          {BAUD_LABELS.map((label, idx) => (
            <TouchableOpacity
              key={`ch2-${idx}`}
              style={[styles.baudButton, can2Baud === idx ? styles.baudActive : null]}
              onPress={() => setCan2Baud(idx)}
            >
              <Text style={[styles.baudText, can2Baud === idx ? styles.baudTextActive : null]}>
                {label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* SD Card */}
      <View style={styles.section}>
        <Text style={styles.subtitle}>SD Card Recording</Text>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.actionButton, recording ? styles.recordingButton : styles.recordButton]}
            onPress={toggleRecording}
            disabled={!connected}
          >
            <Text style={styles.actionButtonText}>
              {recording ? '⏹ Stop Recording' : '⏺ Start Recording'}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* LED Control */}
      <View style={styles.section}>
        <Text style={styles.subtitle}>LED Presets</Text>
        {LED_PRESETS.map((preset, idx) => (
          <TouchableOpacity
            key={idx}
            style={styles.presetButton}
            onPress={() => {
              preset.colors.forEach((c, i) => setLed(i, c.r, c.g, c.b));
            }}
            disabled={!connected}
          >
            <View style={[styles.ledIndicator, { backgroundColor: `rgb(${preset.colors[0].r},${preset.colors[0].g},${preset.colors[0].b})` }]} />
            <Text style={styles.presetText}>{preset.name}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Firmware Update */}
      <View style={styles.section}>
        <TouchableOpacity style={styles.dfuButton} onPress={enterDFU} disabled={!connected}>
          <Text style={styles.dfuButtonText}>🔄 Enter DFU Mode</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0D1117', padding: 12 },
  title: { color: '#E6EDF3', fontSize: 18, fontWeight: 'bold', marginBottom: 16 },
  section: { marginBottom: 20 },
  subtitle: { color: '#8B949E', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  statusRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  statusDot: { width: 12, height: 12, borderRadius: 6, marginRight: 8 },
  dotGreen: { backgroundColor: '#3FB950' },
  dotRed: { backgroundColor: '#F85149' },
  statusText: { color: '#C9D1D9', fontSize: 16 },
  connectButton: { borderRadius: 8, paddingVertical: 12, alignItems: 'center' },
  connectStyle: { backgroundColor: '#1F6FEB' },
  disconnectStyle: { backgroundColor: '#21262D', borderWidth: 1, borderColor: '#F85149' },
  connectButtonText: { color: '#FFFFFF', fontSize: 14, fontWeight: 'bold' },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  infoLabel: { color: '#8B949E', fontSize: 13 },
  infoValue: { color: '#E6EDF3', fontSize: 13, fontFamily: 'Courier' },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  label: { color: '#C9D1D9', fontSize: 13, width: 40 },
  baudButton: {
    paddingHorizontal: 10, paddingVertical: 6,
    backgroundColor: '#21262D', borderRadius: 4, marginHorizontal: 3,
  },
  baudActive: { backgroundColor: '#1F6FEB' },
  baudText: { color: '#8B949E', fontSize: 11 },
  baudTextActive: { color: '#FFFFFF' },
  actionButton: { borderRadius: 6, paddingVertical: 10, paddingHorizontal: 16, alignItems: 'center' },
  recordButton: { backgroundColor: '#238636' },
  recordingButton: { backgroundColor: '#DA3633' },
  actionButtonText: { color: '#FFFFFF', fontSize: 13, fontWeight: 'bold' },
  presetButton: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#161B22', borderRadius: 6,
    padding: 10, marginBottom: 6, borderWidth: 1, borderColor: '#30363D',
  },
  ledIndicator: { width: 16, height: 16, borderRadius: 8, marginRight: 10 },
  presetText: { color: '#C9D1D9', fontSize: 13 },
  dfuButton: {
    backgroundColor: '#21262D', borderRadius: 8, paddingVertical: 12,
    alignItems: 'center', borderWidth: 1, borderColor: '#F0883E',
  },
  dfuButtonText: { color: '#F0883E', fontSize: 14, fontWeight: 'bold' },
});