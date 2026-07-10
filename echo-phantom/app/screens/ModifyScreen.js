/**
 * ModifyScreen.js — Real-Time Audio Modification
 *
 * Author: jayis1
 * License: MIT
 *
 * Configure real-time DSP filters applied to the audio passing
 * through the implant. Supports low-pass, high-pass, band-pass,
 * gain control, and noise injection.
 */

import React, { useState, useContext } from 'react';
import { View, Text, StyleSheet, Switch } from 'react-native';
import { Card, Title, Paragraph, Button, Slider, RadioButton } from 'react-native-paper';
import { BLEContext } from '../components/BLEManager';

const FILTER_NONE = 0;
const FILTER_LP = 1;
const FILTER_HP = 2;
const FILTER_BP = 3;
const FILTER_CUSTOM = 4;

const FILTER_NAMES = {
  0: 'None (Passthrough)',
  1: 'Low-Pass Filter',
  2: 'High-Pass Filter',
  3: 'Band-Pass Filter',
  4: 'Custom FIR',
};

export default function ModifyScreen() {
  const ble = useContext(BLEContext);
  const [filterType, setFilterType] = useState(FILTER_NONE);
  const [gain, setGain] = useState(100);
  const [noiseEnabled, setNoiseEnabled] = useState(false);
  const [noiseLevel, setNoiseLevel] = useState(10);
  const [cutoffFreq, setCutoffFreq] = useState(8000);
  const [numTaps, setNumTaps] = useState(16);
  const [active, setActive] = useState(false);

  const handleApplyFilter = async () => {
    // Generate FIR coefficients based on filter type
    let coeffs = [];
    if (filterType === FILTER_LP) {
      coeffs = generateLPCoeffs(numTaps, cutoffFreq);
    } else if (filterType === FILTER_HP) {
      coeffs = generateHPCoeffs(numTaps, cutoffFreq);
    } else if (filterType === FILTER_BP) {
      coeffs = generateBPCoeffs(numTaps, cutoffFreq);
    }
    await ble.setFilter(filterType, coeffs);
    await ble.setMode(3); // MODE_MODIFY
    setActive(true);
  };

  const handleDisable = async () => {
    await ble.setFilter(FILTER_NONE, []);
    await ble.setMode(0); // MODE_PASSTHROUGH
    setActive(false);
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Audio Modification</Title>
          <Paragraph style={styles.description}>
            Apply real-time DSP processing to audio passing through the
            implant. The target device will hear the modified audio,
            enabling adversarial audio perturbations and subtle attacks.
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Filter type */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Filter Type</Title>
          <RadioButton.Group
            onValueChange={value => setFilterType(parseInt(value))}
            value={filterType.toString()}
          >
            <View style={styles.radioRow}>
              <RadioButton value={FILTER_NONE.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{FILTER_NAMES[FILTER_NONE]}</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={FILTER_LP.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{FILTER_NAMES[FILTER_LP]}</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={FILTER_HP.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{FILTER_NAMES[FILTER_HP]}</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={FILTER_BP.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{FILTER_NAMES[FILTER_BP]}</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value={FILTER_CUSTOM.toString()} color="#e91e63" />
              <Text style={styles.radioLabel}>{FILTER_NAMES[FILTER_CUSTOM]}</Text>
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      {/* Filter parameters */}
      {filterType !== FILTER_NONE && filterType !== FILTER_CUSTOM && (
        <Card style={styles.card}>
          <Card.Content>
            <Title style={styles.sectionTitle}>Cutoff Frequency: {cutoffFreq} Hz</Title>
            <Slider
              value={cutoffFreq}
              onValueChange={setCutoffFreq}
              minimumValue={100}
              maximumValue={20000}
              step={100}
              color="#e91e63"
              style={styles.slider}
            />
            <Title style={styles.sectionTitle}>Filter Taps: {numTaps}</Title>
            <Slider
              value={numTaps}
              onValueChange={setNumTaps}
              minimumValue={4}
              maximumValue={64}
              step={4}
              color="#e91e63"
              style={styles.slider}
            />
          </Card.Content>
        </Card>
      )}

      {/* Gain control */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Gain: {gain}%</Title>
          <Slider
            value={gain}
            onValueChange={setGain}
            minimumValue={0}
            maximumValue={200}
            step={1}
            color="#e91e63"
            style={styles.slider}
          />
          <Text style={styles.gainHint}>
            100% = unity gain. 0% = silence. 200% = 2x amplification.
          </Text>
        </Card.Content>
      </Card>

      {/* Noise injection */}
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.switchRow}>
            <Title style={styles.sectionTitle}>Noise Injection</Title>
            <Switch
              value={noiseEnabled}
              onValueChange={setNoiseEnabled}
              color="#e91e63"
            />
          </View>
          {noiseEnabled && (
            <>
              <Text style={styles.noiseLabel}>Noise Level: {noiseLevel}%</Text>
              <Slider
                value={noiseLevel}
                onValueChange={setNoiseLevel}
                minimumValue={1}
                maximumValue={100}
                step={1}
                color="#ff9800"
                style={styles.slider}
              />
              <Text style={styles.gainHint}>
                Adds white noise to the audio. Can be used to degrade ASR accuracy
                or test noise-robustness of voice processing pipelines.
              </Text>
            </>
          )}
        </Card.Content>
      </Card>

      {/* Controls */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Controls</Title>
          <View style={styles.buttonRow}>
            <Button
              mode="contained"
              onPress={handleApplyFilter}
              disabled={!ble.connected || filterType === FILTER_NONE}
              style={[styles.actionButton, { backgroundColor: '#4caf50' }]}
            >
              Apply & Activate
            </Button>
            <Button
              mode="contained"
              onPress={handleDisable}
              disabled={!active || !ble.connected}
              style={[styles.actionButton, { backgroundColor: '#f44336' }]}
            >
              Disable
            </Button>
          </View>
          {active && (
            <View style={styles.activeIndicator}>
              <View style={styles.activeDot} />
              <Text style={styles.activeText}>● MODIFYING AUDIO — {FILTER_NAMES[filterType]}</Text>
            </View>
          )}
        </Card.Content>
      </Card>
    </View>
  );
}

// Generate simple LP FIR coefficients (windowed-sinc)
function generateLPCoeffs(nTaps, cutoffHz) {
  const coeffs = [];
  const normalizedCutoff = cutoffHz / 48000.0; // Assume 48kHz sample rate
  const M = nTaps - 1;
  let sum = 0;

  for (let i = 0; i < nTaps; i++) {
    const n = i - Math.floor(M / 2);
    let h;
    if (n === 0) {
      h = 2.0 * normalizedCutoff;
    } else {
      h = Math.sin(2 * Math.PI * normalizedCutoff * n) / (Math.PI * n);
    }
    // Hamming window
    const w = 0.54 - 0.46 * Math.cos(2 * Math.PI * i / M);
    coeffs.push(Math.round(h * w * 32768));
    sum += h * w;
  }

  // Normalize
  if (sum !== 0) {
    for (let i = 0; i < nTaps; i++) {
      coeffs[i] = Math.round((coeffs[i] / 32768 / sum) * 32768);
    }
  }

  return coeffs;
}

// Generate simple HP FIR coefficients
function generateHPCoeffs(nTaps, cutoffHz) {
  const lpCoeffs = generateLPCoeffs(nTaps, cutoffHz);
  // HP = delta - LP
  const hpCoeffs = lpCoeffs.map((c, i) => {
    if (i === Math.floor((nTaps - 1) / 2)) {
      return 32768 - c; // delta - LP coefficient
    }
    return -c;
  });
  return hpCoeffs;
}

// Generate simple BP FIR coefficients (LP * HP shifted)
function generateBPCoeffs(nTaps, centerFreq) {
  const lpCoeffs = generateLPCoeffs(nTaps, centerFreq * 2);
  const hpCoeffs = generateHPCoeffs(nTaps, centerFreq / 2);
  // Convolve LP and HP to get BP
  const bpCoeffs = new Array(nTaps).fill(0);
  for (let i = 0; i < nTaps; i++) {
    for (let j = 0; j < nTaps && i + j < nTaps; j++) {
      bpCoeffs[i + j] += (lpCoeffs[i] * hpCoeffs[j]) >> 15;
    }
  }
  return bpCoeffs;
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0f0f1e' },
  card: { marginBottom: 12, backgroundColor: '#1a1a2e' },
  title: { color: '#e0e0e0', fontSize: 20 },
  sectionTitle: { color: '#e0e0e0', fontSize: 16 },
  description: { color: '#888', fontSize: 13, marginTop: 8 },
  radioRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  radioLabel: { color: '#e0e0e0', fontSize: 14 },
  slider: { marginTop: 8 },
  gainHint: { color: '#888', fontSize: 12, marginTop: 4 },
  switchRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  noiseLabel: { color: '#e0e0e0', fontSize: 14, marginTop: 8 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 12 },
  actionButton: { flex: 1, marginHorizontal: 4 },
  activeIndicator: { flexDirection: 'row', alignItems: 'center', justifyContent: 'center', marginTop: 12 },
  activeDot: { width: 8, height: 8, borderRadius: 4, backgroundColor: '#4caf50', marginRight: 8 },
  activeText: { color: '#4caf50', fontWeight: 'bold', fontSize: 14 },
});