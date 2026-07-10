/**
 * CaptureScreen.js — Audio Capture Control
 *
 * Author: jayis1
 * License: MIT
 *
 * Start/stop audio capture from the I²S/TDM bus, select capture
 * destination (SD card, BLE stream, USB), and view live capture
 * statistics.
 */

import React, { useState, useContext, useEffect } from 'react';
import { View, Text, StyleSheet, Switch, Alert } from 'react-native';
import { Card, Title, Paragraph, Button, RadioButton, ProgressBar } from 'react-native-paper';
import { BLEContext } from '../components/BLEManager';

const CAP_DEST_SD = 0;
const CAP_DEST_BLE = 1;
const CAP_DEST_USB = 2;
const CAP_DEST_SDRAM = 3;

export default function CaptureScreen() {
  const ble = useContext(BLEContext);
  const [capturing, setCapturing] = useState(false);
  const [destination, setDestination] = useState(CAP_DEST_SD);
  const [stats, setStats] = useState({ frames: 0, bytes: 0, fillPct: 0 });

  // Update stats from BLE status
  useEffect(() => {
    if (ble.status) {
      setStats({
        frames: ble.status.captureFrames || 0,
        bytes: ble.status.captureBytes || 0,
        fillPct: ble.status.bufferFill || 0,
      });
    }
  }, [ble.status]);

  const handleStartCapture = async () => {
    const result = await ble.startCapture(destination);
    if (result) {
      setCapturing(true);
    }
  };

  const handleStopCapture = async () => {
    const result = await ble.stopCapture();
    if (result) {
      setCapturing(false);
    }
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Audio Capture</Title>
          <Paragraph style={styles.description}>
            Capture audio from the I²S/TDM bus at the physical layer,
            bypassing all OS-level privacy controls including software
            mute, wake-word gating, and app-level permissions.
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Destination selector */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Capture Destination</Title>
          <RadioButton.Group
            onValueChange={value => setDestination(parseInt(value))}
            value={destination.toString()}
          >
            <View style={styles.radioRow}>
              <RadioButton value={CAP_DEST_SD.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>SD Card (WAV file)</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={CAP_DEST_BLE.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>BLE Stream (live)</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={CAP_DEST_USB.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>USB CDC (host PC)</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={CAP_DEST_SDRAM.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>SDRAM buffer (on-device)</Text>
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      {/* Capture controls */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Controls</Title>
          <View style={styles.buttonRow}>
            <Button
              mode="contained"
              onPress={handleStartCapture}
              disabled={capturing || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#4caf50' }]}
              icon="mic"
            >
              Start Capture
            </Button>
            <Button
              mode="contained"
              onPress={handleStopCapture}
              disabled={!capturing || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#f44336' }]}
              icon="stop"
            >
              Stop Capture
            </Button>
          </View>
          {capturing && (
            <View style={styles.activeIndicator}>
              <View style={styles.recordingDot} />
              <Text style={styles.recordingText}>● CAPTURING</Text>
            </View>
          )}
        </Card.Content>
      </Card>

      {/* Capture statistics */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Capture Statistics</Title>
          <View style={styles.statsGrid}>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Frames Captured</Text>
              <Text style={styles.statValue}>{stats.frames.toLocaleString()}</Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Data Captured</Text>
              <Text style={styles.statValue}>{formatBytes(stats.bytes)}</Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Buffer Fill</Text>
              <Text style={styles.statValue}>{stats.fillPct}%</Text>
            </View>
          </View>
          {destination === CAP_DEST_SDRAM && (
            <ProgressBar
              progress={stats.fillPct / 100}
              color="#2196f3"
              style={styles.progressBar}
            />
          )}
        </Card.Content>
      </Card>

      <Card style={styles.warningCard}>
        <Card.Content>
          <Paragraph style={styles.warningText}>
            ⚠ Ensure you have explicit written authorization to capture
            audio from the target device. Unauthorized audio interception
            may violate wiretap statutes (18 U.S.C. § 2511) and privacy laws.
          </Paragraph>
        </Card.Content>
      </Card>
    </View>
  );
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
  sectionTitle: { color: '#e0e0e0', fontSize: 16 },
  description: { color: '#888', fontSize: 13, marginTop: 8 },
  radioRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  radioLabel: { color: '#e0e0e0', fontSize: 14 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 12 },
  actionButton: { flex: 1, marginHorizontal: 4 },
  activeIndicator: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', marginTop: 12 },
  recordingDot: { width: 8, height: 8, borderRadius: 4, backgroundColor: '#f44336', marginRight: 8 },
  recordingText: { color: '#f44336', fontWeight: 'bold', fontSize: 14 },
  statsGrid: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 8 },
  statItem: { width: '50%', marginBottom: 8 },
  statLabel: { color: '#888', fontSize: 12 },
  statValue: { color: '#e0e0e0', fontSize: 16, fontWeight: 'bold' },
  progressBar: { marginTop: 8, height: 6 },
  warningCard: { marginTop: 8, backgroundColor: '#2a1a1a' },
  warningText: { color: '#ff9800', fontSize: 12 },
});