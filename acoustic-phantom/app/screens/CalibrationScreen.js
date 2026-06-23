/**
 * ACOUSTIC-PHANTOM — Calibration Screen
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Guided per-keyboard calibration wizard. The operator presses each
 * key 20 times in a guided sequence. The device collects labeled
 * feature vectors and trains a nearest-centroid classifier on-device.
 * This screen sends the key labels and monitors calibration progress.
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView,
  ProgressViewIOS, ProgressBarAndroid, Platform,
} from 'react-native';
import { useBLE } from '../utils/ble';
import { CMD, KEY_LABELS } from '../utils/protocol';

// Calibration keys (standard QWERTY layout, 47 keys)
const CALIB_KEYS = [
  'Q','W','E','R','T','Y','U','I','O','P',
  'A','S','D','F','G','H','J','K','L',
  'Z','X','C','V','B','N','M',
  '1','2','3','4','5','6','7','8','9','0',
  'SPACE','ENTER','SHIFT','TAB','ESC',
  '-','=','[',']','\\',';','\'',',','.','/',
];

const SAMPLES_PER_KEY = 20;

export default function CalibrationScreen() {
  const ble = useBLE();
  const [calibrating, setCalibrating] = useState(false);
  const [currentKeyIdx, setCurrentKeyIdx] = useState(0);
  const [currentSample, setCurrentSample] = useState(0);
  const [results, setResults] = useState({}); // key -> accuracy

  const currentKey = CALIB_KEYS[currentKeyIdx];
  const overallProgress = calibrating
    ? (currentKeyIdx * SAMPLES_PER_KEY + currentSample) /
      (CALIB_KEYS.length * SAMPLES_PER_KEY)
    : 0;

  // Monitor calibration progress notifications
  useEffect(() => {
    if (!ble.calibProgress) return;
    // calibProgress = { current: sample_in_current_key, total: total_samples }
    if (calibrating) {
      setCurrentSample(ble.calibProgress.current);
      if (ble.calibProgress.current >= SAMPLES_PER_KEY) {
        // Move to next key
        if (currentKeyIdx < CALIB_KEYS.length - 1) {
          setCurrentKeyIdx(idx => idx + 1);
          setCurrentSample(0);
        } else {
          // Calibration complete
          ble.sendCommand(CMD.FINISH_CALIBRATION);
          setCalibrating(false);
        }
      }
    }
  }, [ble.calibProgress]);

  const startCalibration = () => {
    setCalibrating(true);
    setCurrentKeyIdx(0);
    setCurrentSample(0);
    setResults({});
    ble.sendCommand(CMD.START_CALIBRATION);
  };

  const sendSampleLabel = (keyLabel) => {
    // Find the key ID from KEY_LABELS
    const classId = KEY_LABELS.indexOf(keyLabel);
    if (classId < 0) return;

    // Encode: [class_id(1)] [feat_dim(2)] — the feature vector
    // is captured on-device from the live audio; we only send the label.
    const data = new Uint8Array(3);
    data[0] = classId & 0xFF;
    data[1] = 39;  // MFCC feature dim (13 + 13 delta + 13 delta-delta)
    data[2] = 0;
    ble.sendCommand(CMD.CALIBRATION_SAMPLE, Array.from(data));
  };

  const finishCalibration = () => {
    ble.sendCommand(CMD.FINISH_CALIBRATION);
    setCalibrating(false);
  };

  const cancelCalibration = () => {
    setCalibrating(false);
    setCurrentKeyIdx(0);
    setCurrentSample(0);
    ble.sendCommand(CMD.DISARM);
  };

  const renderProgressBar = (progress) => {
    if (Platform.OS === 'ios') {
      return <ProgressViewIOS progress={progress} progressTintColor="#2196F3" />;
    }
    return <ProgressBarAndroid progress={progress} color="#2196F3" styleAttr="Horizontal" />;
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.title}>Keyboard Calibration</Text>
        <Text style={styles.description}>
          Train a per-keyboard classifier by pressing each key {SAMPLES_PER_KEY}×.
          This adapts the acoustic model to your specific keyboard and environment.
          Takes approximately 5 minutes.
        </Text>
      </View>

      {!calibrating ? (
        <View style={styles.section}>
          <TouchableOpacity
            style={[styles.button, styles.primaryButton]}
            onPress={startCalibration}
            disabled={!ble.connected}
          >
            <Text style={styles.buttonText}>
              {ble.connected ? 'Start Calibration' : 'Connect device first'}
            </Text>
          </TouchableOpacity>
        </View>
      ) : (
        <>
          {/* Current key prompt */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Press this key {SAMPLES_PER_KEY} times:</Text>
            <View style={styles.keyPromptContainer}>
              <Text style={styles.keyPrompt}>{currentKey}</Text>
            </View>
            <Text style={styles.sampleCount}>
              Sample {currentSample + 1} / {SAMPLES_PER_KEY}
            </Text>
            {renderProgressBar(currentSample / SAMPLES_PER_KEY)}

            <TouchableOpacity
              style={[styles.button, styles.recordButton]}
              onPress={() => sendSampleLabel(currentKey)}
            >
              <Text style={styles.buttonText}>Record Sample</Text>
            </TouchableOpacity>
          </View>

          {/* Overall progress */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>
              Overall: Key {currentKeyIdx + 1} / {CALIB_KEYS.length}
            </Text>
            {renderProgressBar(overallProgress)}
          </View>

          {/* Remaining keys */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Key Sequence</Text>
            <View style={styles.keyGrid}>
              {CALIB_KEYS.map((key, idx) => (
                <View
                  key={key}
                  style={[
                    styles.keyChip,
                    idx < currentKeyIdx && styles.keyChipDone,
                    idx === currentKeyIdx && styles.keyChipCurrent,
                  ]}
                >
                  <Text style={[
                    styles.keyChipText,
                    idx < currentKeyIdx && styles.keyChipTextDone,
                    idx === currentKeyIdx && styles.keyChipTextCurrent,
                  ]}>
                    {key}
                  </Text>
                </View>
              ))}
            </View>
          </View>

          {/* Cancel/Finish */}
          <View style={styles.buttonRow}>
            <TouchableOpacity
              style={[styles.button, styles.dangerButton]}
              onPress={cancelCalibration}
            >
              <Text style={styles.buttonText}>Cancel</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.button, styles.secondaryButton]}
              onPress={finishCalibration}
            >
              <Text style={styles.buttonText}>Finish Early</Text>
            </TouchableOpacity>
          </View>
        </>
      )}

      <View style={styles.footer}>
        <Text style={styles.footerText}>jayis1 — ACOUSTIC-PHANTOM</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a1a' },
  section: { padding: 16, borderBottomWidth: 1, borderBottomColor: '#333' },
  title: { color: '#FFF', fontSize: 22, fontWeight: 'bold', marginBottom: 8 },
  description: { color: '#999', fontSize: 14, lineHeight: 20 },
  sectionTitle: {
    color: '#888', fontSize: 14, fontWeight: 'bold',
    textTransform: 'uppercase', marginBottom: 12,
  },
  button: {
    padding: 14, borderRadius: 8, alignItems: 'center', flex: 1,
  },
  primaryButton: { backgroundColor: '#2196F3' },
  secondaryButton: { backgroundColor: '#555' },
  recordButton: { backgroundColor: '#4CAF50', marginTop: 12 },
  dangerButton: { backgroundColor: '#F44336' },
  buttonText: { color: '#FFF', fontSize: 16, fontWeight: '600' },
  buttonRow: { flexDirection: 'row', gap: 12, padding: 16 },
  keyPromptContainer: {
    backgroundColor: '#222',
    borderRadius: 12,
    padding: 30,
    alignItems: 'center',
    marginBottom: 12,
  },
  keyPrompt: {
    color: '#4CAF50',
    fontSize: 48,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  sampleCount: { color: '#CCC', fontSize: 16, marginBottom: 8 },
  keyGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  keyChip: {
    width: 36, height: 36,
    backgroundColor: '#333',
    borderRadius: 6,
    justifyContent: 'center',
    alignItems: 'center',
  },
  keyChipDone: { backgroundColor: '#1B5E20' },
  keyChipCurrent: { backgroundColor: '#2196F3' },
  keyChipText: { color: '#888', fontSize: 14, fontFamily: 'monospace' },
  keyChipTextDone: { color: '#4CAF50' },
  keyChipTextCurrent: { color: '#FFF', fontWeight: 'bold' },
  footer: { padding: 20, alignItems: 'center' },
  footerText: { color: '#555', fontSize: 12 },
});