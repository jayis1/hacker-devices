/**
 * NFC Relay Phantom — Settings Screen
 * Device configuration, firmware update, info
 */

import React, { useState } from 'react';
import { View, ScrollView, StyleSheet, Alert } from 'react-native';
import { Text, Card, Button, Switch, Divider, List, TextInput } from 'react-native-paper';
import { bleManager } from '../App';

export default function SettingsScreen() {
  const [autoScan, setAutoScan] = useState(true);
  const [autoReconnect, setAutoReconnect] = useState(false);
  const [debugMode, setDebugMode] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState({
    firmware: '1.0.0',
    hardware: 'Rev A',
    bleMac: 'XX:XX:XX:XX:XX:XX',
    nfcFw: 'PN5180 v1.2',
  });

  const handleFactoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will erase all saved keys and settings. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: async () => {
            await bleManager.sendCommand('SETTINGS', 'FACTORY_RESET', {});
          },
        },
      ]
    );
  };

  const handleFirmwareUpdate = () => {
    Alert.alert(
      'Firmware Update',
      'Check for firmware updates? The device will restart after updating.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Update',
          onPress: async () => {
            await bleManager.sendCommand('SETTINGS', 'FW_UPDATE', {});
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* Device Info */}
      <Card style={styles.card}>
        <Card.Title title="Device Info" />
        <Card.Content>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Firmware:</Text>
            <Text style={styles.infoValue}>{deviceInfo.firmware}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Hardware:</Text>
            <Text style={styles.infoValue}>{deviceInfo.hardware}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>BLE MAC:</Text>
            <Text style={styles.infoValue}>{deviceInfo.bleMac}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>NFC FW:</Text>
            <Text style={styles.infoValue}>{deviceInfo.nfcFw}</Text>
          </View>
          <Button
            mode="outlined"
            onPress={handleFirmwareUpdate}
            icon="update"
            style={styles.button}
          >
            Check for Updates
          </Button>
        </Card.Content>
      </Card>

      {/* BLE Settings */}
      <Card style={styles.card}>
        <Card.Title title="BLE Settings" />
        <Card.Content>
          <List.Item
            title="Auto-Scan on Launch"
            description="Automatically scan for Phantom devices when app opens"
            left={(props) => <Switch value={autoScan} onValueChange={setAutoScan} />}
          />
          <List.Item
            title="Auto-Reconnect"
            description="Reconnect to last device automatically"
            left={(props) => <Switch value={autoReconnect} onValueChange={setAutoReconnect} />}
          />
        </Card.Content>
      </Card>

      {/* NFC Settings */}
      <Card style={styles.card}>
        <Card.Title title="NFC Settings" />
        <Card.Content>
          <List.Item
            title="Debug Mode"
            description="Log raw NFC frames to USB serial"
            left={(props) => <Switch value={debugMode} onValueChange={setDebugMode} />}
          />
        </Card.Content>
      </Card>

      {/* Security */}
      <Card style={styles.card}>
        <Card.Title title="Security" />
        <Card.Content>
          <Text style={styles.securityNote}>
            All BLE communication is encrypted with AES-128-GCM. Mifare keys
            are stored in battery-backed SRAM and wiped on tamper detect.
          </Text>
          <Button
            mode="outlined"
            onPress={() => {/* Clear stored keys */}}
            icon="key-remove"
            color="#F44336"
            style={styles.button}
          >
            Clear Stored Keys
          </Button>
        </Card.Content>
      </Card>

      {/* Danger Zone */}
      <Card style={[styles.card, styles.dangerCard]}>
        <Card.Title title="Danger Zone" titleStyle={{ color: '#F44336' }} />
        <Card.Content>
          <Button
            mode="contained"
            onPress={handleFactoryReset}
            color="#F44336"
            icon="restore"
          >
            Factory Reset
          </Button>
        </Card.Content>
      </Card>

      {/* About */}
      <Card style={styles.card}>
        <Card.Title title="About" />
        <Card.Content>
          <Text style={styles.aboutText}>
            NFC Relay Phantom v1.0{'\n'}
            © 2026 Hacker Devices{'\n'}
            Licensed under GPL-2.0{'\n'}
            {'\n'}
            Open-source hardware and software for authorized{'\n'}
            security research and penetration testing.{'\n'}
            {'\n'}
            STM32L4S5VIT6 · NRF52832 · PN5180 · EM4095{'\n'}
            BOM: ~$38 | Form Factor: 85×54mm (credit card)
          </Text>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  card: {
    margin: 8,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  infoLabel: {
    fontSize: 14,
    color: '#666',
  },
  infoValue: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333',
  },
  button: {
    marginVertical: 8,
  },
  securityNote: {
    fontSize: 12,
    color: '#666',
    marginBottom: 8,
    lineHeight: 18,
  },
  dangerCard: {
    borderWidth: 1,
    borderColor: '#F44336',
  },
  aboutText: {
    fontSize: 13,
    lineHeight: 20,
    color: '#333',
    fontFamily: 'monospace',
  },
});