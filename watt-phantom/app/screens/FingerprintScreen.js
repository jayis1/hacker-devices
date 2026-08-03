/**
 * FingerprintScreen.js — USB-C PD device fingerprinting
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Captures and displays the PD negotiation fingerprint of a connected
 * USB-C device. The fingerprint includes PDO request patterns, timing
 * histograms, and current draw profiles. The device is matched against
 * a database of known profiles for identification.
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, ActivityIndicator } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function FingerprintScreen() {
  const { sendCommand } = useDevice();
  const [capturing, setCapturing] = useState(false);
  const [result, setResult] = useState(null);
  const [matchResult, setMatchResult] = useState(null);
  const [error, setError] = useState('');

  const captureFingerprint = async () => {
    setCapturing(true);
    setError('');
    setResult(null);
    setMatchResult(null);

    const resp = await sendCommand('FINGERPRINT_CAPTURE', 2000);
    if (resp && resp.startsWith('OK')) {
      // Wait for capture to complete (PD negotiation takes ~200-500ms)
      setTimeout(async () => {
        setCapturing(false);
        // Now try to match
        const matchResp = await sendCommand('FINGERPRINT_MATCH', 3000);
        if (matchResp && matchResp.startsWith('OK')) {
          // Parse: OK FINGERPRINT class=1 (iPhone 15 Pro)
          const match = matchResp.match(/class=(\d+) \((.+)\)/);
          if (match) {
            setMatchResult({
              deviceClass: parseInt(match[1]),
              deviceName: match[2],
            });
          }
        } else {
          setMatchResult({ deviceClass: 0, deviceName: 'Unknown' });
        }
        setResult('Capture complete');
      }, 1000);
    } else {
      setCapturing(false);
      setError(resp || 'Capture failed');
    }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Device Fingerprint</Text>
      <Text style={styles.subtitle}>USB-C PD negotiation fingerprinting & identification</Text>

      <View style={styles.infoCard}>
        <Text style={styles.infoText}>
          WattPhantom captures a feature vector from the PD negotiation sequence
          of a connected USB-C device, including:
        </Text>
        <Text style={styles.bullet}>• PDO types and request order</Text>
        <Text style={styles.bullet}>• Inter-message timing histogram</Text>
        <Text style={styles.bullet}>• Total negotiation time</Text>
        <Text style={styles.bullet}>• Initial and steady-state current draw</Text>
        <Text style={styles.bullet}>• Message count and protocol patterns</Text>
      </View>

      <TouchableOpacity
        style={[styles.captureButton, capturing && styles.captureButtonActive]}
        onPress={captureFingerprint}
        disabled={capturing}
      >
        {capturing ? (
          <View style={styles.loadingRow}>
            <ActivityIndicator color="#0d1117" size="small" />
            <Text style={styles.captureButtonText}>Capturing...</Text>
          </View>
        ) : (
          <Text style={styles.captureButtonText}>Capture Fingerprint</Text>
        )}
      </TouchableOpacity>

      {error ? <Text style={styles.errorText}>{error}</Text> : null}

      {result && (
        <View style={styles.resultCard}>
          <Text style={styles.resultTitle}>Capture Result</Text>
          <Text style={styles.resultText}>{result}</Text>
        </View>
      )}

      {matchResult && (
        <View style={styles.matchCard}>
          <Text style={styles.matchTitle}>Device Match</Text>
          <Text style={styles.matchDevice}>{matchResult.deviceName}</Text>
          <Text style={styles.matchClass}>Class: {matchResult.deviceClass}</Text>
          {matchResult.deviceClass === 0 && (
            <Text style={styles.matchUnknown}>
              No match found in database. Device may be unknown or a clone.
            </Text>
          )}
        </View>
      )}

      <Text style={styles.disclaimer}>
        ⚠️ Device fingerprinting is for authorized security assessment only.
        Using fingerprints to track individuals without consent may violate
        privacy laws in your jurisdiction.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#00d4aa' },
  subtitle: { fontSize: 12, color: '#8b949e', marginBottom: 16 },
  infoCard: { backgroundColor: '#161b22', padding: 12, borderRadius: 8, marginBottom: 16, borderWidth: 1, borderColor: '#30363d' },
  infoText: { color: '#8b949e', fontSize: 12, marginBottom: 8 },
  bullet: { color: '#6e7681', fontSize: 11, marginLeft: 8, marginBottom: 2 },
  captureButton: { backgroundColor: '#00d4aa', padding: 16, borderRadius: 8, alignItems: 'center', marginBottom: 16 },
  captureButtonActive: { backgroundColor: '#21262d' },
  captureButtonText: { color: '#0d1117', fontWeight: 'bold', fontSize: 16 },
  loadingRow: { flexDirection: 'row', gap: 8, alignItems: 'center' },
  errorText: { color: '#f85149', fontSize: 12, marginBottom: 8 },
  resultCard: { backgroundColor: '#161b22', padding: 12, borderRadius: 8, marginBottom: 8, borderWidth: 1, borderColor: '#30363d' },
  resultTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  resultText: { color: '#e6edf3', fontSize: 13 },
  matchCard: { backgroundColor: '#0d2818', padding: 16, borderRadius: 8, marginBottom: 8, borderWidth: 1, borderColor: '#00d4aa' },
  matchTitle: { color: '#00d4aa', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  matchDevice: { color: '#e6edf3', fontSize: 20, fontWeight: 'bold', marginBottom: 4 },
  matchClass: { color: '#8b949e', fontSize: 12 },
  matchUnknown: { color: '#f85149', fontSize: 11, marginTop: 8 },
  disclaimer: { color: '#f85149', fontSize: 10, marginTop: 16, textAlign: 'center' },
});