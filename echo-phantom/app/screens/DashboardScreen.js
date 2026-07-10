/**
 * DashboardScreen.js — Device Status Dashboard
 *
 * Author: jayis1
 * License: MIT
 *
 * Shows device connection status, battery, current operating mode,
 * detected audio format, and capture statistics.
 */

import React, { useState, useEffect, useContext } from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { Card, Title, Paragraph, Button, ProgressBar, IconButton } from 'react-native-paper';
import { BLEContext } from '../components/BLEManager';

const MODE_NAMES = {
  0: 'PASSTHROUGH',
  1: 'CAPTURE',
  2: 'INJECT',
  3: 'MODIFY',
  4: 'OFFLINE',
};

const PROTOCOL_NAMES = {
  0: 'I²S Philips',
  1: 'Left-Justified',
  2: 'Right-Justified',
  3: 'PCM Short',
  4: 'PCM Long',
  5: 'TDM',
};

export default function DashboardScreen() {
  const ble = useContext(BLEContext);
  const [lastUpdate, setLastUpdate] = useState(null);

  useEffect(() => {
    // Poll status every 2 seconds when connected
    if (ble.connected) {
      const interval = setInterval(() => {
        ble.getStatus();
        setLastUpdate(new Date().toLocaleTimeString());
      }, 2000);
      return () => clearInterval(interval);
    }
  }, [ble.connected]);

  if (!ble.connected) {
    return (
      <View style={styles.container}>
        <Card style={styles.card}>
          <Card.Content>
            <Title style={styles.title}>ECHO-Phantom</Title>
            <Paragraph style={styles.subtitle}>I²S/TDM Audio Bus MITM</Paragraph>
            <Paragraph style={styles.author}>Author: jayis1</Paragraph>
          </Card.Content>
        </Card>

        <Card style={styles.card}>
          <Card.Content>
            <Title>Not Connected</Title>
            <Paragraph>Scan for and connect to an ECHO-Phantom device.</Paragraph>
          </Card.Content>
          <Card.Actions>
            <Button
              mode="contained"
              onPress={ble.scanAndConnect}
              loading={ble.scanning}
              style={styles.button}
            >
              {ble.scanning ? 'Scanning...' : 'Scan & Connect'}
            </Button>
          </Card.Actions>
        </Card>

        {ble.error && (
          <Card style={styles.errorCard}>
            <Card.Content>
              <Paragraph style={styles.error}>{ble.error}</Paragraph>
            </Card.Content>
          </Card>
        )}

        <Card style={styles.disclaimerCard}>
          <Card.Content>
            <Paragraph style={styles.disclaimer}>
              ⚠ AUTHORIZED SECURITY RESEARCH USE ONLY
            </Paragraph>
            <Paragraph style={styles.disclaimerText}>
              This device is designed exclusively for authorized security
              research and penetration testing. Unauthorized audio
              interception may violate wiretap statutes, surveillance laws,
              and privacy regulations.
            </Paragraph>
          </Card.Content>
        </Card>
      </View>
    );
  }

  const s = ble.status || {};

  return (
    <View style={styles.container}>
      {/* Connection Status */}
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.row}>
            <Title style={styles.title}>ECHO-Phantom</Title>
            <View style={[styles.statusDot, styles.statusConnected]} />
          </View>
          <Paragraph style={styles.statusText}>Connected via BLE 5.0</Paragraph>
          {lastUpdate && <Paragraph style={styles.muted}>Last update: {lastUpdate}</Paragraph>}
        </Card.Content>
      </Card>

      {/* Device Status */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Device Status</Title>
          <View style={styles.statusGrid}>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Mode</Text>
              <Text style={styles.statusValue}>{MODE_NAMES[s.mode] || 'Unknown'}</Text>
            </View>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Uptime</Text>
              <Text style={styles.statusValue}>{formatUptime(s.uptime)}</Text>
            </View>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>SD Card</Text>
              <Text style={[styles.statusValue, { color: s.sdPresent ? '#4caf50' : '#f44336' }]}>
                {s.sdPresent ? 'Present' : 'Absent'}
              </Text>
            </View>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>USB</Text>
              <Text style={[styles.statusValue, { color: s.usbConnected ? '#4caf50' : '#888' }]}>
                {s.usbConnected ? 'Connected' : 'Disconnected'}
              </Text>
            </View>
          </View>
        </Card.Content>
      </Card>

      {/* Audio Format */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Detected Audio Format</Title>
          <View style={styles.formatGrid}>
            <View style={styles.formatItem}>
              <Text style={styles.formatLabel}>Sample Rate</Text>
              <Text style={styles.formatValue}>{(s.sampleRate / 1000).toFixed(1)} kHz</Text>
            </View>
            <View style={styles.formatItem}>
              <Text style={styles.formatLabel}>Bit Depth</Text>
              <Text style={styles.formatValue}>{s.bitDepth}-bit</Text>
            </View>
            <View style={styles.formatItem}>
              <Text style={styles.formatLabel}>Channels</Text>
              <Text style={styles.formatValue}>{s.channels}</Text>
            </View>
            <View style={styles.formatItem}>
              <Text style={styles.formatLabel}>Protocol</Text>
              <Text style={styles.formatValue}>{PROTOCOL_NAMES[s.protocol] || 'Unknown'}</Text>
            </View>
          </View>
        </Card.Content>
      </Card>

      {/* Battery */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Battery</Title>
          <View style={styles.batteryRow}>
            <Text style={styles.batteryText}>{s.batteryPct}%</Text>
            <Text style={styles.batteryVoltage}>{s.batteryMv} mV</Text>
            {s.charging && <Text style={styles.charging}>⚡ Charging</Text>}
          </View>
          <ProgressBar
            progress={(s.batteryPct || 0) / 100}
            color={s.batteryPct < 20 ? '#f44336' : s.batteryPct < 50 ? '#ff9800' : '#4caf50'}
            style={styles.progressBar}
          />
        </Card.Content>
      </Card>

      {/* Capture Stats */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Capture Statistics</Title>
          <View style={styles.statusGrid}>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Frames</Text>
              <Text style={styles.statusValue}>{(s.captureFrames || 0).toLocaleString()}</Text>
            </View>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Data</Text>
              <Text style={styles.statusValue}>{formatBytes(s.captureBytes)}</Text>
            </View>
            <View style={styles.statusItem}>
              <Text style={styles.statusLabel}>Buffer</Text>
              <Text style={styles.statusValue}>{s.bufferFill || 0}%</Text>
            </View>
          </View>
          <ProgressBar
            progress={(s.bufferFill || 0) / 100}
            color="#2196f3"
            style={styles.progressBar}
          />
        </Card.Content>
      </Card>

      <Button
        mode="outlined"
        onPress={ble.disconnect}
        style={styles.disconnectButton}
      >
        Disconnect
      </Button>
    </View>
  );
}

function formatUptime(seconds) {
  if (!seconds) return '--';
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return `${h}h ${m}m ${s}s`;
}

function formatBytes(bytes) {
  if (!bytes) return '0 B';
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1073741824) return `${(bytes / 1048576).toFixed(1)} MB`;
  return `${(bytes / 1073741824).toFixed(2)} GB`;
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0f0f1e' },
  card: { marginBottom: 12, backgroundColor: '#1a1a2e' },
  title: { color: '#e0e0e0', fontSize: 20 },
  subtitle: { color: '#888', fontSize: 14 },
  author: { color: '#666', fontSize: 12, marginTop: 4 },
  button: { marginTop: 8, backgroundColor: '#e91e63' },
  statusText: { color: '#4caf50', fontSize: 14, marginTop: 4 },
  muted: { color: '#666', fontSize: 12 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  statusDot: { width: 12, height: 12, borderRadius: 6 },
  statusConnected: { backgroundColor: '#4caf50' },
  statusGrid: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 8 },
  statusItem: { width: '50%', marginBottom: 8 },
  statusLabel: { color: '#888', fontSize: 12 },
  statusValue: { color: '#e0e0e0', fontSize: 16, fontWeight: 'bold' },
  formatGrid: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 8 },
  formatItem: { width: '50%', marginBottom: 8 },
  formatLabel: { color: '#888', fontSize: 12 },
  formatValue: { color: '#2196f3', fontSize: 16, fontWeight: 'bold' },
  batteryRow: { flexDirection: 'row', alignItems: 'center', marginTop: 8 },
  batteryText: { color: '#4caf50', fontSize: 24, fontWeight: 'bold' },
  batteryVoltage: { color: '#888', fontSize: 14, marginLeft: 12 },
  charging: { color: '#ff9800', fontSize: 14, marginLeft: 12 },
  progressBar: { marginTop: 8, height: 6 },
  disconnectButton: { marginTop: 8, borderColor: '#f44336' },
  errorCard: { marginBottom: 12, backgroundColor: '#3a1a1a' },
  error: { color: '#f44336' },
  disclaimerCard: { marginTop: 16, backgroundColor: '#2a1a1a' },
  disclaimer: { color: '#ff9800', fontWeight: 'bold', fontSize: 14 },
  disclaimerText: { color: '#888', fontSize: 12, marginTop: 8 },
});