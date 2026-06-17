/**
 * TapConfigScreen.js — FiberPhantom Tap Configuration
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Configure the optical tap: APD bias voltage, AGC, calibration,
 * deployment mode selection, and link type display.
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  TextInput, Switch, Alert, Slider
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { C2_MSG_TYPE } from '../utils/protocol';

export default function TapConfigScreen({ ble, connected, status }) {
  const [apdBias, setApdBias] = useState(65000); // mV
  const [agcEnabled, setAgcEnabled] = useState(true);
  const [calibrating, setCalibrating] = useState(false);
  const [selectedMode, setSelectedMode] = useState(0);

  const sendApdBias = async () => {
    if (!connected) return;
    try {
      await ble.setApdBias(apdBias);
      Alert.alert('APD Bias', `Set to ${apdBias} mV`);
    } catch (e) {
      Alert.alert('Error', e.message);
    }
  };

  const runCalibration = async () => {
    if (!connected) return;
    setCalibrating(true);
    try {
      await ble.autoCalibrate();
      // Result comes back via CALIB_RESULT message in App.js
      Alert.alert('Calibration', 'Auto-calibration started. Result will appear shortly.');
    } catch (e) {
      Alert.alert('Error', e.message);
    } finally {
      setCalibrating(false);
    }
  };

  const toggleAgc = async (value) => {
    setAgcEnabled(value);
    if (connected) {
      try {
        await ble.setAgc(value);
      } catch (e) {
        console.error(e);
      }
    }
  };

  const changeMode = async (mode) => {
    setSelectedMode(mode);
    if (connected) {
      try {
        await ble.setMode(mode);
        const modeNames = ['Bend-Tap', 'Inline-Couple', 'SFP+ Pass-Through', 'Standby'];
        Alert.alert('Mode Changed', `Deployment mode set to: ${modeNames[mode]}`);
      } catch (e) {
        Alert.alert('Error', e.message);
      }
    }
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection warning */}
      {!connected && (
        <View style={styles.warning}>
          <Icon name="alert-circle" size={20} color="#f85149" />
          <Text style={styles.warningText}>Not connected to device</Text>
        </View>
      )}

      {/* Deployment Mode */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Deployment Mode</Text>
        <Text style={styles.cardDesc}>
          Select the optical tap deployment mode. Bend-Tap is read-only (non-invasive).
          Inline-Couple and SFP+ modes support MITM injection.
        </Text>
        <View style={styles.modeGrid}>
          {[
            { mode: 0, name: 'Bend-Tap', icon: 'gesture-tap', desc: 'Non-invasive, read-only' },
            { mode: 1, name: 'Inline-Couple', icon: 'merge', desc: 'Splice + MITM injection' },
            { mode: 2, name: 'SFP+ Pass-Through', icon: 'ethernet', desc: 'Full transparent access' },
            { mode: 3, name: 'Standby', icon: 'power-plug-off', desc: 'Idle / charging' },
          ].map(item => (
            <TouchableOpacity
              key={item.mode}
              style={[
                styles.modeButton,
                selectedMode === item.mode && styles.modeButtonActive,
              ]}
              onPress={() => changeMode(item.mode)}
              disabled={!connected}
            >
              <Icon name={item.icon} size={28} color={selectedMode === item.mode ? '#fff' : '#8b949e'} />
              <Text style={[styles.modeName, selectedMode === item.mode && styles.modeNameActive]}>
                {item.name}
              </Text>
              <Text style={styles.modeDesc}>{item.desc}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* APD Bias Control */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>APD Bias Voltage</Text>
        <Text style={styles.cardDesc}>
          Controls the reverse bias on the Avalanche Photodiode. Higher bias = more gain
          but increased noise. Use auto-calibrate for optimal SNR.
        </Text>
        <View style={styles.biasDisplay}>
          <Text style={styles.biasValue}>{(apdBias / 1000).toFixed(1)} V</Text>
          <Text style={styles.biasRange}>Range: 30.0 – 80.0 V</Text>
        </View>
        <Slider
          style={styles.slider}
          minimumValue={30000}
          maximumValue={80000}
          step={1000}
          value={apdBias}
          onValueChange={setApdBias}
          onSlidingComplete={sendApdBias}
          minimumTrackTintColor="#00ff88"
          maximumTrackTintColor="#30363d"
          thumbTintColor="#00ff88"
          disabled={!connected}
        />
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.button, styles.buttonPrimary]}
            onPress={sendApdBias}
            disabled={!connected}
          >
            <Text style={styles.buttonText}>Apply Bias</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.buttonSecondary]}
            onPress={runCalibration}
            disabled={!connected || calibrating}
          >
            <Text style={styles.buttonText}>
              {calibrating ? 'Calibrating...' : 'Auto-Calibrate'}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* AGC Toggle */}
      <View style={styles.card}>
        <View style={styles.row}>
          <View>
            <Text style={styles.cardTitle}>Auto-Gain Control (AGC)</Text>
            <Text style={styles.cardDesc}>
              Automatically adjusts APD bias to maintain optimal signal level.
            </Text>
          </View>
          <Switch
            value={agcEnabled}
            onValueChange={toggleAgc}
            trackColor={{ false: '#30363d', true: '#238636' }}
            thumbColor={agcEnabled ? '#00ff88' : '#8b949e'}
            disabled={!connected}
          />
        </View>
      </View>

      {/* Link Information */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Optical Link</Text>
        <View style={styles.row}>
          <Text style={styles.label}>Detected Rate</Text>
          <Text style={[styles.value, { color: status.linkRate !== 'Down' ? '#00ff88' : '#f85149' }]}>
            {status.linkRate}
          </Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Protocol</Text>
          <Text style={styles.value}>
            {status.linkRate.startsWith('FC') ? 'Fibre Channel' : 'Ethernet'}
          </Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>FPGA Status</Text>
          <Text style={[styles.value, { color: status.fpgaReady ? '#00ff88' : '#f85149' }]}>
            {status.fpgaReady ? 'CDR Locked' : 'Not Ready'}
          </Text>
        </View>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1117',
    padding: 12,
  },
  warning: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    backgroundColor: '#3d1014',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  warningText: {
    color: '#f85149',
    fontSize: 14,
  },
  card: {
    backgroundColor: '#161b22',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#30363d',
  },
  cardTitle: {
    color: '#e6edf3',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  cardDesc: {
    color: '#8b949e',
    fontSize: 12,
    marginBottom: 12,
    lineHeight: 18,
  },
  modeGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  modeButton: {
    flex: 1,
    minWidth: '45%',
    backgroundColor: '#21262d',
    borderRadius: 10,
    padding: 16,
    alignItems: 'center',
    borderWidth: 2,
    borderColor: 'transparent',
  },
  modeButtonActive: {
    backgroundColor: '#1a3d28',
    borderColor: '#00ff88',
  },
  modeName: {
    color: '#8b949e',
    fontSize: 14,
    fontWeight: 'bold',
    marginTop: 8,
  },
  modeNameActive: {
    color: '#fff',
  },
  modeDesc: {
    color: '#6e7681',
    fontSize: 10,
    marginTop: 4,
    textAlign: 'center',
  },
  biasDisplay: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'baseline',
    marginBottom: 8,
  },
  biasValue: {
    color: '#00ff88',
    fontSize: 28,
    fontWeight: 'bold',
  },
  biasRange: {
    color: '#8b949e',
    fontSize: 12,
  },
  slider: {
    height: 40,
    marginBottom: 8,
  },
  buttonRow: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 8,
  },
  button: {
    flex: 1,
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
  },
  buttonPrimary: {
    backgroundColor: '#238636',
  },
  buttonSecondary: {
    backgroundColor: '#1f6feb',
  },
  buttonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  label: {
    color: '#8b949e',
    fontSize: 14,
  },
  value: {
    color: '#e6edf3',
    fontSize: 14,
    fontWeight: '500',
  },
});