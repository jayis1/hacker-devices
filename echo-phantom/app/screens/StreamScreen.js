/**
 * StreamScreen.js — Live Audio Monitoring
 *
 * Author: jayis1
 * License: MIT
 *
 * Listen to captured microphone audio in real time over BLE.
 * The audio is compressed (µ-law or PCM) and played through
 * the phone's speaker or headphones.
 */

import React, { useState, useContext, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Alert } from 'react-native';
import { Card, Title, Paragraph, Button, RadioButton, Slider } from 'react-native-paper';
import { BLEContext } from '../components/BLEManager';

const QUALITY_8KBPS = 8;
const QUALITY_16KBPS = 16;
const QUALITY_32KBPS = 32;

export default function StreamScreen() {
  const ble = useContext(BLEContext);
  const [streaming, setStreaming] = useState(false);
  const [quality, setQuality] = useState(QUALITY_16KBPS);
  const [volume, setVolume] = useState(80);
  const [packetCount, setPacketCount] = useState(0);
  const [signalLevel, setSignalLevel] = useState(-60);
  const packetCountRef = useRef(0);

  // Monitor incoming audio data
  useEffect(() => {
    if (ble.audioData) {
      packetCountRef.current++;
      setPacketCount(packetCountRef.current);

      // Calculate signal level (RMS) from the audio data
      let sum = 0;
      for (let i = 0; i < ble.audioData.length; i++) {
        const sample = ble.audioData[i] - 128; // Center around 0
        sum += sample * sample;
      }
      const rms = Math.sqrt(sum / Math.max(1, ble.audioData.length));
      const db = rms > 0 ? 20 * Math.log10(rms / 128) : -60;
      setSignalLevel(Math.max(-60, db));
    }
  }, [ble.audioData]);

  const handleStartStream = async () => {
    const result = await ble.startStream(quality);
    if (result) {
      setStreaming(true);
      packetCountRef.current = 0;
    }
  };

  const handleStopStream = async () => {
    const result = await ble.stopStream();
    if (result) {
      setStreaming(false);
    }
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Live Audio Stream</Title>
          <Paragraph style={styles.description}>
            Listen to the captured microphone audio in real time.
            Audio is compressed and sent over BLE 5.0 from the implant
            to your phone.
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Quality selector */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Stream Quality</Title>
          <RadioButton.Group
            onValueChange={value => setQuality(parseInt(value))}
            value={quality.toString()}
          >
            <View style={styles.radioRow}>
              <RadioButton value={QUALITY_8KBPS.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>8 kbps (Low — µ-law, 8kHz mono)</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={QUALITY_16KBPS.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>16 kbps (Medium — µ-law, 16kHz mono)</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={QUALITY_32KBPS.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>32 kbps (High — 16-bit PCM, 16kHz mono)</Text>
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      {/* Volume control */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Playback Volume: {volume}%</Title>
          <Slider
            value={volume}
            onValueChange={setVolume}
            minimumValue={0}
            maximumValue={100}
            step={1}
            color="#e91e63"
            style={styles.slider}
          />
        </Card.Content>
      </Card>

      {/* Stream controls */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Controls</Title>
          <View style={styles.buttonRow}>
            <Button
              mode="contained"
              onPress={handleStartStream}
              disabled={streaming || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#4caf50' }]}
              icon="hearing"
            >
              Start Stream
            </Button>
            <Button
              mode="contained"
              onPress={handleStopStream}
              disabled={!streaming || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#f44336' }]}
              icon="stop"
            >
              Stop Stream
            </Button>
          </View>
          {streaming && (
            <View style={styles.activeIndicator}>
              <View style={styles.streamDot} />
              <Text style={styles.streamText}>● STREAMING — {quality} kbps</Text>
            </View>
          )}
        </Card.Content>
      </Card>

      {/* Live metrics */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Live Metrics</Title>
          <View style={styles.metricsGrid}>
            <View style={styles.metricItem}>
              <Text style={styles.metricLabel}>Packets Received</Text>
              <Text style={styles.metricValue}>{packetCount.toLocaleString()}</Text>
            </View>
            <View style={styles.metricItem}>
              <Text style={styles.metricLabel}>Signal Level</Text>
              <Text style={styles.metricValue}>{signalLevel.toFixed(1)} dB</Text>
            </View>
            <View style={styles.metricItem}>
              <Text style={styles.metricLabel}>Quality</Text>
              <Text style={styles.metricValue}>{quality} kbps</Text>
            </View>
            <View style={styles.metricItem}>
              <Text style={styles.metricLabel}>Volume</Text>
              <Text style={styles.metricValue}>{volume}%</Text>
            </View>
          </View>

          {/* Signal level meter */}
          <Text style={styles.meterLabel}>Signal Meter</Text>
          <View style={styles.meterContainer}>
            <View
              style={[
                styles.meterBar,
                {
                  width: `${Math.max(0, Math.min(100, (signalLevel + 60) / 60 * 100))}%`,
                  backgroundColor: signalLevel > -20 ? '#4caf50' : signalLevel > -40 ? '#ff9800' : '#666',
                },
              ]}
            />
          </View>
        </Card.Content>
      </Card>

      <Card style={styles.warningCard}>
        <Card.Content>
          <Paragraph style={styles.warningText}>
            ⚠ Ensure you have authorization to monitor audio from the target
            device. Live audio monitoring of private conversations without
            consent is illegal in most jurisdictions.
          </Paragraph>
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0f0f1e' },
  card: { marginBottom: 12, backgroundColor: '#1a1a2e' },
  title: { color: '#e0e0e0', fontSize: 20 },
  sectionTitle: { color: '#e0e0e0', fontSize: 16 },
  description: { color: '#888', fontSize: 13, marginTop: 8 },
  radioRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  radioLabel: { color: '#e0e0e0', fontSize: 13 },
  slider: { marginTop: 8 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 12 },
  actionButton: { flex: 1, marginHorizontal: 4 },
  activeIndicator: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', marginTop: 12 },
  streamDot: { width: 8, height: 8, borderRadius: 4, backgroundColor: '#4caf50', marginRight: 8 },
  streamText: { color: '#4caf50', fontWeight: 'bold', fontSize: 14 },
  metricsGrid: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 8 },
  metricItem: { width: '50%', marginBottom: 8 },
  metricLabel: { color: '#888', fontSize: 12 },
  metricValue: { color: '#e0e0e0', fontSize: 16, fontWeight: 'bold' },
  meterLabel: { color: '#888', fontSize: 12, marginTop: 8 },
  meterContainer: { height: 12, backgroundColor: '#2a2a3e', borderRadius: 6, marginTop: 4, overflow: 'hidden' },
  meterBar: { height: '100%', borderRadius: 6 },
  warningCard: { marginTop: 8, backgroundColor: '#2a1a1a' },
  warningText: { color: '#ff9800', fontSize: 12 },
});