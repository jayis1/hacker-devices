/**
 * SettingsScreen.js — Device Settings
 *
 * Author: jayis1
 * License: MIT
 *
 * BLE pairing, firmware info, factory reset, and legal/ethical
 * disclaimer acknowledgment.
 */

import React, { useState, useContext } from 'react';
import { View, Text, StyleSheet, Alert, Linking, ScrollView } from 'react-native';
import { Card, Title, Paragraph, Button, Divider, List, Switch } from 'react-native-paper';
import { BLEContext } from '../components/BLEManager';

export default function SettingsScreen() {
  const ble = useContext(BLEContext);
  const [disclaimerAccepted, setDisclaimerAccepted] = useState(false);
  const [autoReconnect, setAutoReconnect] = useState(true);
  const [encryptedOnly, setEncryptedOnly] = useState(true);

  const handleFactoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will erase all inject clips and reset the device to factory defaults. The device will reboot. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          onPress: () => {
            ble.factoryReset();
            Alert.alert('Reset', 'Factory reset command sent. Device will reboot.');
          },
          style: 'destructive',
        },
      ]
    );
  };

  const handlePing = async () => {
    await ble.sendCommand(0x01); // CMD_PING
    Alert.alert('Ping', 'Ping sent to device. Check dashboard for response.');
  };

  return (
    <ScrollView style={styles.container}>
      {/* Device info */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>ECHO-Phantom Settings</Title>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Firmware Version</Text>
            <Text style={styles.infoValue}>1.0.0</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Author</Text>
            <Text style={styles.infoValue}>jayis1</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>License</Text>
            <Text style={styles.infoValue}>CERN-OHL-S v2 / GPL-2.0 / MIT</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Status</Text>
            <Text style={[styles.infoValue, { color: ble.connected ? '#4caf50' : '#f44336' }]}>
              {ble.connected ? 'Connected' : 'Disconnected'}
            </Text>
          </View>
        </Card.Content>
      </Card>

      {/* Connection settings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Connection</Title>
          <List.Item
            title="Auto-reconnect"
            description="Automatically reconnect to last device"
            right={() => (
              <Switch value={autoReconnect} onValueChange={setAutoReconnect} color="#e91e63" />
            )}
          />
          <Divider />
          <List.Item
            title="Encrypted BLE only"
            description="Require AES-256 encrypted BLE link"
            right={() => (
              <Switch value={encryptedOnly} onValueChange={setEncryptedOnly} color="#e91e63" />
            )}
          />
          <Divider />
          <Button mode="outlined" onPress={handlePing} style={styles.actionButton} color="#e91e63">
            Ping Device
          </Button>
          <Button
            mode="outlined"
            onPress={() => ble.scanAndConnect()}
            style={styles.actionButton}
            color="#e91e63"
            loading={ble.scanning}
          >
            {ble.scanning ? 'Scanning...' : 'Scan & Connect'}
          </Button>
          <Button
            mode="outlined"
            onPress={ble.disconnect}
            style={styles.actionButton}
            color="#f44336"
            disabled={!ble.connected}
          >
            Disconnect
          </Button>
        </Card.Content>
      </Card>

      {/* Diagnostics */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Diagnostics</Title>
          <Button
            mode="outlined"
            onPress={() => ble.getStatus()}
            style={styles.actionButton}
            color="#e91e63"
            disabled={!ble.connected}
          >
            Query Status
          </Button>
          <Button
            mode="outlined"
            onPress={() => ble.getBattery()}
            style={styles.actionButton}
            color="#e91e63"
            disabled={!ble.connected}
          >
            Check Battery
          </Button>
          <Button
            mode="outlined"
            onPress={() => ble.sendCommand(0x03)}
            style={styles.actionButton}
            color="#e91e63"
            disabled={!ble.connected}
          >
            Query Audio Format
          </Button>
        </Card.Content>
      </Card>

      {/* Danger zone */}
      <Card style={[styles.card, styles.dangerCard]}>
        <Card.Content>
          <Title style={styles.dangerTitle}>⚠ Danger Zone</Title>
          <Paragraph style={styles.dangerDescription}>
            Factory reset will erase all stored inject clips and restore
            default settings. The device will reboot immediately.
          </Paragraph>
          <Button
            mode="contained"
            onPress={handleFactoryReset}
            style={styles.dangerButton}
            color="#f44336"
            disabled={!ble.connected}
          >
            Factory Reset
          </Button>
        </Card.Content>
      </Card>

      {/* Legal & Ethical Disclaimer */}
      <Card style={[styles.card, styles.legalCard]}>
        <Card.Content>
          <Title style={styles.legalTitle}>Legal & Ethical Disclaimer</Title>
          <Paragraph style={styles.legalText}>
            ECHO-Phantom is designed exclusively for authorized security
            research, penetration testing with explicit written consent,
            and academic study of embedded audio pipeline security on
            devices you own or have explicit written authorization to
            assess.
          </Paragraph>
          <Paragraph style={styles.legalText}>
            Intercepting audio from devices, persons, or premises you do
            not own or do not have authorization to test may violate wiretap
            statutes (e.g., 18 U.S.C. § 2511), computer fraud and abuse laws
            (18 U.S.C. § 1030), surveillance and eavesdropping laws, and
            biometric/voice-data protection regulations.
          </Paragraph>
          <Paragraph style={styles.legalText}>
            Injecting synthetic audio into a device's microphone stream can
            cause a target system to act on false voice commands, which may
            constitute unauthorized access to a protected computer.
          </Paragraph>
          <Paragraph style={styles.legalText}>
            The author (jayis1) assumes no liability for any misuse. Always
            obtain proper written authorization before deployment.
          </Paragraph>
          <Divider style={styles.divider} />
          <List.Item
            title="I have read and understand the disclaimer"
            titleStyle={disclaimerAccepted ? styles.acceptedText : styles.unacceptedText}
            right={() => (
              <Switch
                value={disclaimerAccepted}
                onValueChange={setDisclaimerAccepted}
                color="#4caf50"
              />
            )}
          />
          <Text style={styles.author}>Designed by jayis1 — 2026</Text>
        </Card.Content>
      </Card>

      <View style={styles.bottomPadding} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  card: { marginHorizontal: 16, marginVertical: 8, backgroundColor: '#1a1a2e' },
  title: { color: '#e0e0e0', fontSize: 20 },
  sectionTitle: { color: '#e0e0e0', fontSize: 16, marginBottom: 8 },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  infoLabel: { color: '#888', fontSize: 14 },
  infoValue: { color: '#e0e0e0', fontSize: 14, fontWeight: 'bold' },
  actionButton: { marginTop: 8 },
  dangerCard: { backgroundColor: '#2a1a1a' },
  dangerTitle: { color: '#f44336', fontSize: 18 },
  dangerDescription: { color: '#ff9800', fontSize: 13, marginTop: 8 },
  dangerButton: { marginTop: 12, backgroundColor: '#f44336' },
  legalCard: { backgroundColor: '#1a1a2e' },
  legalTitle: { color: '#ff9800', fontSize: 18 },
  legalText: { color: '#aaa', fontSize: 12, marginTop: 8 },
  divider: { marginVertical: 12, backgroundColor: '#333' },
  acceptedText: { color: '#4caf50', fontSize: 14 },
  unacceptedText: { color: '#888', fontSize: 14 },
  author: { color: '#666', fontSize: 11, textAlign: 'center', marginTop: 12 },
  bottomPadding: { height: 40 },
});