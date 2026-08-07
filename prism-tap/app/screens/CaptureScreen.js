/**
 * screens/CaptureScreen.js — Frame Capture Control Screen
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Switch, TextInput, ScrollView } from 'react-native';
import { useDevice } from '../utils/deviceContext';
import { CMD, MODE, FMT, FMT_NAMES, buildStartCapturePayload, buildSetModePayload } from '../utils/protocol';

const CaptureScreen = () => {
  const { connected, sendCommand, refreshStatus, framesCaptured, batteryPct, sdPresent, csiLinkUp, mode } = useDevice();

  const [capturing, setCapturing] = useState(false);
  const [format, setFormat] = useState(FMT.RAW10);
  const [width, setWidth] = useState('1920');
  const [height, setHeight] = useState('1080');
  const [interval, setIntervalMs] = useState('0');
  const [maxFrames, setMaxFrames] = useState('0');
  const [jpegCompress, setJpegCompress] = useState(false);
  const [frameCount, setFrameCount] = useState(0);

  // Auto-refresh status every 2 seconds when capturing
  useEffect(() => {
    if (!capturing) return;
    const timer = setInterval(() => {
      refreshStatus();
      setFrameCount(framesCaptured);
    }, 2000);
    return () => clearInterval(timer);
  }, [capturing, refreshStatus, framesCaptured]);

  const handleStartCapture = async () => {
    if (!connected) return;

    // First set mode to CAPTURE_ONLY or FULL_MITM
    await sendCommand(CMD.SET_MODE, buildSetModePayload(MODE.CAPTURE_ONLY));

    // Then start capture
    const payload = buildStartCapturePayload(
      format,
      parseInt(width) || 1920,
      parseInt(height) || 1080,
      parseInt(interval) || 0,
      parseInt(maxFrames) || 0,
      jpegCompress
    );
    await sendCommand(CMD.START_CAPTURE, payload);
    setCapturing(true);
  };

  const handleStopCapture = async () => {
    await sendCommand(CMD.STOP_CAPTURE, []);
    setCapturing(false);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.statusBar}>
        <Text style={styles.statusText}>
          Mode: {mode || '---'} | Battery: {batteryPct}% | SD: {sdPresent ? '✓' : '✗'}
        </Text>
        <Text style={styles.linkText}>
          CSI Link: {csiLinkUp ? '✓ UP' : '✗ DOWN'} | Frames: {frameCount}
        </Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Capture Configuration</Text>

        <View style={styles.row}>
          <Text style={styles.label}>Frame Format:</Text>
          <View style={styles.formatButtons}>
            {Object.entries(FMT_NAMES).map(([key, name]) => (
              <TouchableOpacity
                key={key}
                style={[styles.formatButton, format === parseInt(key) && styles.formatButtonActive]}
                onPress={() => setFormat(parseInt(key))}
              >
                <Text style={[styles.formatText, format === parseInt(key) && styles.formatTextActive]}>
                  {name}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        <View style={styles.row}>
          <Text style={styles.label}>Width:</Text>
          <TextInput
            style={styles.input}
            value={width}
            onChangeText={setWidth}
            keyboardType="numeric"
            placeholder="1920"
          />
        </View>

        <View style={styles.row}>
          <Text style={styles.label}>Height:</Text>
          <TextInput
            style={styles.input}
            value={height}
            onChangeText={setHeight}
            keyboardType="numeric"
            placeholder="1080"
          />
        </View>

        <View style={styles.row}>
          <Text style={styles.label}>Interval (ms):</Text>
          <TextInput
            style={styles.input}
            value={interval}
            onChangeText={setIntervalMs}
            keyboardType="numeric"
            placeholder="0 = every frame"
          />
        </View>

        <View style={styles.row}>
          <Text style={styles.label}>Max Frames:</Text>
          <TextInput
            style={styles.input}
            value={maxFrames}
            onChangeText={setMaxFrames}
            keyboardType="numeric"
            placeholder="0 = unlimited"
          />
        </View>

        <View style={styles.row}>
          <Text style={styles.label}>JPEG Compress:</Text>
          <Switch
            value={jpegCompress}
            onValueChange={setJpegCompress}
            trackColor={{ false: '#333', true: '#00d9ff' }}
          />
        </View>
      </View>

      <View style={styles.actionSection}>
        {!capturing ? (
          <TouchableOpacity
            style={[styles.actionButton, styles.startButton, !connected && styles.disabled]}
            onPress={handleStartCapture}
            disabled={!connected}
          >
            <Text style={styles.actionText}>▶ Start Capture</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity
            style={[styles.actionButton, styles.stopButton]}
            onPress={handleStopCapture}
          >
            <Text style={styles.actionText}>■ Stop Capture</Text>
          </TouchableOpacity>
        )}

        {capturing && (
          <View style={styles.liveStats}>
            <Text style={styles.liveLabel}>CAPTURING...</Text>
            <Text style={styles.liveCount}>{frameCount} frames captured</Text>
          </View>
        )}
      </View>

      <View style={styles.infoBox}>
        <Text style={styles.infoTitle}>About Capture</Text>
        <Text style={styles.infoText}>
          Captures raw MIPI CSI-2 camera frames or DSI display frames to the
          SD card in PTF (Prism-Tap Frame) format. Each frame includes a
          40-byte header with metadata (dimensions, format, timestamp, CRC32).
          Use JPEG compression to reduce storage by ~10:1.
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
  statusBar: {
    padding: 12,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  statusText: {
    color: '#888',
    fontSize: 11,
  },
  linkText: {
    color: '#00d9ff',
    fontSize: 11,
    marginTop: 2,
  },
  section: {
    padding: 15,
  },
  sectionTitle: {
    color: '#00d9ff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 10,
  },
  label: {
    color: '#aaa',
    fontSize: 14,
    width: 120,
  },
  input: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    color: '#fff',
    borderWidth: 1,
    borderColor: '#333',
    borderRadius: 5,
    paddingHorizontal: 10,
    paddingVertical: 6,
    fontSize: 14,
  },
  formatButtons: {
    flex: 1,
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  formatButton: {
    padding: 5,
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
    fontSize: 11,
  },
  formatTextActive: {
    color: '#00d9ff',
  },
  actionSection: {
    padding: 15,
    alignItems: 'center',
  },
  actionButton: {
    width: '100%',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  startButton: {
    backgroundColor: '#2e7d32',
  },
  stopButton: {
    backgroundColor: '#c62828',
  },
  disabled: {
    opacity: 0.4,
  },
  actionText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  liveStats: {
    marginTop: 15,
    alignItems: 'center',
  },
  liveLabel: {
    color: '#4caf50',
    fontSize: 14,
    fontWeight: 'bold',
  },
  liveCount: {
    color: '#aaa',
    fontSize: 12,
    marginTop: 4,
  },
  infoBox: {
    margin: 15,
    padding: 12,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  infoTitle: {
    color: '#00d9ff',
    fontSize: 13,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  infoText: {
    color: '#888',
    fontSize: 11,
    lineHeight: 16,
  },
});

export default CaptureScreen;