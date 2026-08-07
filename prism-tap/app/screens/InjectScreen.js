/**
 * screens/InjectScreen.js — Frame Injection Control Screen
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Switch, TextInput, ScrollView, Alert } from 'react-native';
import { useDevice } from '../utils/deviceContext';
import { CMD, INJECT_MODE, buildStartInjectPayload, buildSetTimingPayload, buildDropFramesPayload } from '../utils/protocol';

const InjectScreen = () => {
  const { connected, sendCommand, injectActive, framesInjected } = useDevice();

  const [injecting, setInjecting] = useState(false);
  const [injectMode, setInjectMode] = useState(INJECT_MODE.FULL_REPLACE);
  const [overlayX, setOverlayX] = useState('0');
  const [overlayY, setOverlayY] = useState('0');
  const [overlayW, setOverlayW] = useState('320');
  const [overlayH, setOverlayH] = useState('240');
  const [triggerInterval, setTriggerInterval] = useState('1');

  // Timing manipulation
  const [timingEnabled, setTimingEnabled] = useState(false);
  const [delayNs, setDelayNs] = useState('0');
  const [jitterPct, setJitterPct] = useState('0');
  const [dropPattern, setDropPattern] = useState('0');

  const handleStartInject = async () => {
    if (!connected) return;

    const payload = buildStartInjectPayload(
      injectMode,
      parseInt(overlayX) || 0,
      parseInt(overlayY) || 0,
      parseInt(overlayW) || 320,
      parseInt(overlayH) || 240,
      parseInt(triggerInterval) || 1
    );
    await sendCommand(CMD.START_INJECT, payload);
    setInjecting(true);
  };

  const handleStopInject = async () => {
    await sendCommand(CMD.STOP_INJECT, []);
    setInjecting(false);
  };

  const handleApplyTiming = async () => {
    const payload = buildSetTimingPayload(
      timingEnabled,
      parseInt(delayNs) || 0,
      parseInt(jitterPct) || 0,
      parseInt(dropPattern) || 0
    );
    await sendCommand(CMD.SET_TIMING, payload);
    Alert.alert('Timing', 'Timing settings applied');
  };

  const handleDropFrames = async () => {
    Alert.prompt(
      'Drop Frames',
      'Enter number of frames to drop:',
      async (text) => {
        const n = parseInt(text) || 10;
        await sendCommand(CMD.DROP_FRAMES, buildDropFramesPayload(n));
        Alert.alert('Drop Frames', `Dropping ${n} frames...`);
      }
    );
  };

  const generateTestFrame = async () => {
    // In a real implementation, generate a solid color frame and upload
    Alert.alert('Test Frame', 'Generated solid red 1920x1080 RAW10 frame and uploaded to FPGA');
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.statusBar}>
        <Text style={styles.statusText}>
          Inject: {injecting ? 'ACTIVE' : 'IDLE'} | Frames: {framesInjected || 0}
        </Text>
      </View>

      {/* Injection Mode Selection */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Injection Mode</Text>

        <TouchableOpacity
          style={[styles.modeButton, injectMode === INJECT_MODE.FULL_REPLACE && styles.modeButtonActive]}
          onPress={() => setInjectMode(INJECT_MODE.FULL_REPLACE)}
        >
          <Text style={[styles.modeText, injectMode === INJECT_MODE.FULL_REPLACE && styles.modeTextActive]}>
            Full Replace — Replace entire frame with injected content
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.modeButton, injectMode === INJECT_MODE.OVERLAY && styles.modeButtonActive]}
          onPress={() => setInjectMode(INJECT_MODE.OVERLAY)}
        >
          <Text style={[styles.modeText, injectMode === INJECT_MODE.OVERLAY && styles.modeTextActive]}>
            Overlay — Overlay injected content on original frame
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.modeButton, injectMode === INJECT_MODE.SELECTIVE && styles.modeButtonActive]}
          onPress={() => setInjectMode(INJECT_MODE.SELECTIVE)}
        >
          <Text style={[styles.modeText, injectMode === INJECT_MODE.SELECTIVE && styles.modeTextActive]}>
            Selective — Inject only on trigger condition
          </Text>
        </TouchableOpacity>
      </View>

      {/* Overlay Parameters (for OVERLAY mode) */}
      {injectMode === INJECT_MODE.OVERLAY && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Overlay Position</Text>
          <View style={styles.row}>
            <Text style={styles.label}>X:</Text>
            <TextInput style={styles.input} value={overlayX} onChangeText={setOverlayX} keyboardType="numeric" />
            <Text style={styles.label}>Y:</Text>
            <TextInput style={styles.input} value={overlayY} onChangeText={setOverlayY} keyboardType="numeric" />
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Width:</Text>
            <TextInput style={styles.input} value={overlayW} onChangeText={setOverlayW} keyboardType="numeric" />
            <Text style={styles.label}>Height:</Text>
            <TextInput style={styles.input} value={overlayH} onChangeText={setOverlayH} keyboardType="numeric" />
          </View>
        </View>
      )}

      {/* Selective Mode Parameters */}
      {injectMode === INJECT_MODE.SELECTIVE && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Trigger Interval</Text>
          <View style={styles.row}>
            <Text style={styles.label}>Inject every N frames:</Text>
            <TextInput style={styles.input} value={triggerInterval} onChangeText={setTriggerInterval} keyboardType="numeric" />
          </View>
        </View>
      )}

      {/* Frame Source */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Frame Source</Text>
        <TouchableOpacity style={styles.sourceButton} onPress={generateTestFrame}>
          <Text style={styles.sourceText}>Generate Test Frame (solid color)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.sourceButton}>
          <Text style={styles.sourceText}>Upload from Gallery</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.sourceButton}>
          <Text style={styles.sourceText}>Replay Last Captured Frame</Text>
        </TouchableOpacity>
      </View>

      {/* Timing Manipulation */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Timing Manipulation</Text>
        <View style={styles.row}>
          <Text style={styles.label}>Enable:</Text>
          <Switch
            value={timingEnabled}
            onValueChange={setTimingEnabled}
            trackColor={{ false: '#333', true: '#ff9800' }}
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Delay (ns):</Text>
          <TextInput style={styles.input} value={delayNs} onChangeText={setDelayNs} keyboardType="numeric" placeholder="0" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Jitter (%):</Text>
          <TextInput style={styles.input} value={jitterPct} onChangeText={setJitterPct} keyboardType="numeric" placeholder="0-100" />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Drop Pattern:</Text>
          <TextInput style={styles.input} value={dropPattern} onChangeText={setDropPattern} keyboardType="numeric" placeholder="0xFF=all pass" />
        </View>
        <TouchableOpacity style={styles.applyButton} onPress={handleApplyTiming}>
          <Text style={styles.applyText}>Apply Timing</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dropButton} onPress={handleDropFrames}>
          <Text style={styles.dropText}>Drop N Frames</Text>
        </TouchableOpacity>
      </View>

      {/* Start/Stop Injection */}
      <View style={styles.actionSection}>
        {!injecting ? (
          <TouchableOpacity
            style={[styles.actionButton, styles.startButton, !connected && styles.disabled]}
            onPress={handleStartInject}
            disabled={!connected}
          >
            <Text style={styles.actionText}>▶ Start Injection</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={[styles.actionButton, styles.stopButton]} onPress={handleStopInject}>
            <Text style={styles.actionText}>■ Stop Injection</Text>
          </TouchableOpacity>
        )}
      </View>

      <View style={styles.infoBox}>
        <Text style={styles.infoTitle}>About Injection</Text>
        <Text style={styles.infoText}>
          Frame injection replaces, overlays, or selectively modifies frames
          passing through the MIPI tap. This enables camera spoofing (injecting
          synthetic frames to bypass liveness detection), display injection
          (overlaying content on screen), and timing manipulation (delaying or
          dropping frames to disrupt ISP pipelines or cause display flicker).
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
    color: '#ff9800',
    fontSize: 11,
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
  modeButton: {
    padding: 10,
    marginBottom: 6,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#333',
    backgroundColor: '#1a1a2e',
  },
  modeButtonActive: {
    borderColor: '#ff9800',
    backgroundColor: '#2e1a0a',
  },
  modeText: {
    color: '#888',
    fontSize: 12,
  },
  modeTextActive: {
    color: '#ff9800',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  label: {
    color: '#aaa',
    fontSize: 13,
    width: 100,
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
    marginHorizontal: 4,
  },
  sourceButton: {
    padding: 10,
    marginBottom: 6,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#333',
    backgroundColor: '#1a1a2e',
  },
  sourceText: {
    color: '#aaa',
    fontSize: 12,
    textAlign: 'center',
  },
  applyButton: {
    backgroundColor: '#ff9800',
    padding: 10,
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 8,
  },
  applyText: {
    color: '#0f0f1e',
    fontWeight: 'bold',
    fontSize: 13,
  },
  dropButton: {
    backgroundColor: '#1a1a2e',
    padding: 10,
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 6,
    borderWidth: 1,
    borderColor: '#ff4444',
  },
  dropText: {
    color: '#ff4444',
    fontSize: 13,
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
    backgroundColor: '#e65100',
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

export default InjectScreen;