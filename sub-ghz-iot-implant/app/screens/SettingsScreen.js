/**
 * SettingsScreen.js — Radio & Device Configuration
 * Configure frequency, modulation, power, and operating mode
 */

import React, { useContext, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TextInput,
  TouchableOpacity,
  Switch,
} from 'react-native';
import { BleContext } from '../components/BleContext';
import StatusCard from '../components/StatusCard';

const FREQUENCIES = [
  { label: '315 MHz', value: 315000000 },
  { label: '433 MHz', value: 433920000 },
  { label: '868 MHz (EU)', value: 868000000 },
  { label: '868.4 MHz (Z-Wave EU)', value: 868400000 },
  { label: '908.4 MHz (Z-Wave US)', value: 908400000 },
  { label: '915 MHz (US)', value: 915000000 },
];

const MODULATIONS = [
  { label: 'OOK', value: 0 },
  { label: 'FSK', value: 1 },
  { label: 'GFSK', value: 2 },
  { label: 'O-QPSK (Zigbee)', value: 3 },
];

const MODES = [
  { label: 'Idle', value: 'idle' },
  { label: 'Sub-GHz Sniff', value: 'sniff_subghz' },
  { label: 'Zigbee Sniff', value: 'sniff_zigbee' },
  { label: 'Z-Wave Sniff', value: 'sniff_zwave' },
  { label: 'Replay', value: 'replay' },
  { label: 'Zigbee MITM', value: 'mitm_zigbee' },
  { label: 'Rolling Code', value: 'rolling_capture' },
];

export default function SettingsScreen() {
  const {
    connectionState,
    sendCommand,
    config,
    updateConfig,
  } = useContext(BleContext);

  const [selectedFreq, setSelectedFreq] = useState(config.frequency || 868000000);
  const [selectedMod, setSelectedMod] = useState(config.modulation || 2);
  const [txPower, setTxPower] = useState(config.txPower || 14);
  const [selectedMode, setSelectedMode] = useState(config.mode || 'idle');
  const [channelBW, setChannelBW] = useState(config.channelBW || 200000);
  const [deviation, setDeviation] = useState(config.deviation || 25000);
  const [syncWord, setSyncWord] = useState(config.syncWord || 'AABBCCDD');
  const [autoReplay, setAutoReplay] = useState(false);
  const [regionLock, setRegionLock] = useState(true);

  const applySettings = () => {
    if (connectionState !== 'connected') return;

    updateConfig({
      frequency: selectedFreq,
      modulation: selectedMod,
      txPower,
      mode: selectedMode,
      channelBW,
      deviation,
      syncWord,
      autoReplay,
      regionLock,
    });

    sendCommand('CONFIG', {
      frequency: selectedFreq,
      modulation: selectedMod,
      tx_power: txPower,
      channel_bw: channelBW,
      deviation,
      sync_word: syncWord,
    });
  };

  return (
    <ScrollView style={styles.container}>
      {/* Operating Mode */}
      <StatusCard title="Operating Mode" icon="tune" color="#FF9800">
        <View style={styles.modeGrid}>
          {MODES.map((mode) => (
            <TouchableOpacity
              key={mode.value}
              style={[
                styles.modeButton,
                selectedMode === mode.value && styles.modeButtonActive,
              ]}
              onPress={() => setSelectedMode(mode.value)}
            >
              <Text
                style={[
                  styles.modeText,
                  selectedMode === mode.value && styles.modeTextActive,
                ]}
              >
                {mode.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </StatusCard>

      {/* Frequency Selection */}
      <StatusCard title="Frequency" icon="waveform" color="#2196F3">
        <View style={styles.freqGrid}>
          {FREQUENCIES.map((freq) => (
            <TouchableOpacity
              key={freq.value}
              style={[
                styles.freqButton,
                selectedFreq === freq.value && styles.freqButtonActive,
              ]}
              onPress={() => setSelectedFreq(freq.value)}
            >
              <Text
                style={[
                  styles.freqText,
                  selectedFreq === freq.value && styles.freqTextActive,
                ]}
              >
                {freq.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        {/* Custom frequency input */}
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Custom (Hz):</Text>
          <TextInput
            style={styles.textInput}
            keyboardType="numeric"
            placeholder="e.g., 868000000"
            placeholderTextColor="#757575"
            value={selectedFreq < 1000000 ? String(selectedFreq) : ''}
            onChangeText={(text) => {
              const val = parseInt(text, 10);
              if (!isNaN(val) && val > 0) setSelectedFreq(val);
            }}
          />
        </View>
      </StatusCard>

      {/* Modulation */}
      <StatusCard title="Modulation" icon="signal" color="#9C27B0">
        <View style={styles.modGrid}>
          {MODULATIONS.map((mod) => (
            <TouchableOpacity
              key={mod.value}
              style={[
                styles.modButton,
                selectedMod === mod.value && styles.modButtonActive,
              ]}
              onPress={() => setSelectedMod(mod.value)}
            >
              <Text
                style={[
                  styles.modText,
                  selectedMod === mod.value && styles.modTextActive,
                ]}
              >
                {mod.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </StatusCard>

      {/* TX Power */}
      <StatusCard title="TX Power" icon="transmit-tower" color="#F44336">
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>-20 dBm</Text>
          <Text style={styles.sliderValue}>{txPower} dBm</Text>
          <Text style={styles.sliderLabel}>+20 dBm</Text>
        </View>
        <TextInput
          style={styles.powerInput}
          keyboardType="numeric"
          value={String(txPower)}
          onChangeText={(text) => {
            const val = parseInt(text, 10);
            if (!isNaN(val) && val >= -20 && val <= 20) setTxPower(val);
          }}
        />
        {regionLock && txPower > 14 && (
          <Text style={styles.warningText}>
            ⚠ Region lock active. Max TX power limited to +14 dBm.
          </Text>
        )}
      </StatusCard>

      {/* Advanced Settings */}
      <StatusCard title="Advanced" icon="cog-outline" color="#607D8B">
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Channel BW (Hz):</Text>
          <TextInput
            style={styles.textInput}
            keyboardType="numeric"
            value={String(channelBW)}
            onChangeText={(text) => {
              const val = parseInt(text, 10);
              if (!isNaN(val) && val > 0) setChannelBW(val);
            }}
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Deviation (Hz):</Text>
          <TextInput
            style={styles.textInput}
            keyboardType="numeric"
            value={String(deviation)}
            onChangeText={(text) => {
              const val = parseInt(text, 10);
              if (!isNaN(val) && val > 0) setDeviation(val);
            }}
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Sync Word (hex):</Text>
          <TextInput
            style={styles.textInput}
            value={syncWord}
            onChangeText={setSyncWord}
            maxLength={8}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.inputLabel}>Auto-replay captured</Text>
          <Switch
            value={autoReplay}
            onValueChange={setAutoReplay}
            trackColor={{ false: '#2A2A4E', true: '#00E676' }}
            thumbColor={autoReplay ? '#FFFFFF' : '#757575'}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.inputLabel}>Region lock (legal)</Text>
          <Switch
            value={regionLock}
            onValueChange={setRegionLock}
            trackColor={{ false: '#2A2A4E', true: '#FFC107' }}
            thumbColor={regionLock ? '#FFFFFF' : '#757575'}
          />
        </View>
      </StatusCard>

      {/* Apply Button */}
      <TouchableOpacity
        style={[
          styles.applyButton,
          connectionState !== 'connected' && styles.applyButtonDisabled,
        ]}
        onPress={applySettings}
        disabled={connectionState !== 'connected'}
      >
        <Text style={styles.applyButtonText}>Apply Configuration</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F23',
  },
  modeGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 6,
  },
  modeButton: {
    backgroundColor: '#1A1A3E',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#2A2A4E',
  },
  modeButtonActive: {
    backgroundColor: '#FF9800',
    borderColor: '#FF9800',
  },
  modeText: {
    color: '#B0BEC5',
    fontSize: 12,
    fontWeight: '600',
  },
  modeTextActive: {
    color: '#000000',
  },
  freqGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 6,
  },
  freqButton: {
    backgroundColor: '#1A1A3E',
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#2A2A4E',
  },
  freqButtonActive: {
    backgroundColor: '#2196F3',
    borderColor: '#2196F3',
  },
  freqText: {
    color: '#B0BEC5',
    fontSize: 11,
    fontWeight: '600',
  },
  freqTextActive: {
    color: '#FFFFFF',
  },
  modGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 6,
  },
  modButton: {
    backgroundColor: '#1A1A3E',
    paddingHorizontal: 14,
    paddingVertical: 8,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#2A2A4E',
  },
  modButtonActive: {
    backgroundColor: '#9C27B0',
    borderColor: '#9C27B0',
  },
  modText: {
    color: '#B0BEC5',
    fontSize: 13,
    fontWeight: '600',
  },
  modTextActive: {
    color: '#FFFFFF',
  },
  sliderContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  sliderLabel: {
    color: '#757575',
    fontSize: 12,
  },
  sliderValue: {
    color: '#F44336',
    fontSize: 18,
    fontWeight: '700',
  },
  powerInput: {
    backgroundColor: '#1A1A3E',
    color: '#FFFFFF',
    borderWidth: 1,
    borderColor: '#2A2A4E',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 8,
    fontSize: 16,
    textAlign: 'center',
    marginBottom: 8,
  },
  warningText: {
    color: '#FFC107',
    fontSize: 12,
    fontStyle: 'italic',
  },
  inputRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  inputLabel: {
    color: '#B0BEC5',
    fontSize: 14,
    flex: 1,
  },
  textInput: {
    backgroundColor: '#1A1A3E',
    color: '#FFFFFF',
    borderWidth: 1,
    borderColor: '#2A2A4E',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    fontSize: 14,
    flex: 1,
    textAlign: 'right',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  applyButton: {
    backgroundColor: '#00E676',
    margin: 16,
    paddingVertical: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  applyButtonDisabled: {
    backgroundColor: '#2A2A4E',
  },
  applyButtonText: {
    color: '#000000',
    fontSize: 16,
    fontWeight: '700',
  },
});