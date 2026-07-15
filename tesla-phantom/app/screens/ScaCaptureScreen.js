/**
 * TeslaPhantom — SCA Capture Screen
 * Configure and capture magnetic side-channel traces.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ScaCaptureScreen() {
  const { connected, status, traceData, sendCommand } = useDevice();
  const [samples, setSamples] = useState(16384);
  const [rateKhz, setRateKhz] = useState(800);
  const [gainDb, setGainDb] = useState(24);
  const [filterCutoff, setFilterCutoff] = useState(50000);
  const [capturing, setCapturing] = useState(false);

  const handleCapture = () => {
    setCapturing(true);
    sendCommand({
      cmd: 'sca_capture',
      samples: samples,
      rate_khz: rateKhz,
      gain_db: gainDb
    });
    setTimeout(() => setCapturing(false), 3000);
  };

  const handleAutoGain = () => {
    sendCommand({ cmd: 'calibrate' });
  };

  // Render a simple ASCII-art trace preview from traceData
  const renderTracePreview = () => {
    if (!traceData || traceData.length < 10) return null;
    const chars = '▁▂▃▄▅▆▇█';
    let preview = '';
    const step = Math.max(1, Math.floor(traceData.length / 60));
    for (let i = 0; i < traceData.length && preview.length < 60; i += step) {
      const val = Math.abs(traceData.charCodeAt(i)) / 128;
      const idx = Math.min(7, Math.floor(val * 8));
      preview += chars[idx];
    }
    return (
      <View style={styles.traceBox}>
        <Text style={styles.traceLabel}>Trace Preview:</Text>
        <Text style={styles.traceContent}>{preview}</Text>
        <Text style={styles.traceInfo}>{traceData.length} bytes received</Text>
      </View>
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>SCA Capture Parameters</Text>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Samples:</Text>
        <TextInput
          style={styles.input}
          value={String(samples)}
          onChangeText={(v) => setSamples(parseInt(v) || 1024)}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Rate (kHz):</Text>
        <TextInput
          style={styles.input}
          value={String(rateKhz)}
          onChangeText={(v) => setRateKhz(parseInt(v) || 100)}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>VGA Gain (dB):</Text>
        <TextInput
          style={styles.input}
          value={String(gainDb)}
          onChangeText={(v) => setGainDb(parseInt(v) || 0)}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Filter (Hz):</Text>
        <TextInput
          style={styles.input}
          value={String(filterCutoff)}
          onChangeText={(v) => setFilterCutoff(parseInt(v) || 1000)}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.buttonRow}>
        <TouchableOpacity
          style={[styles.button, styles.captureBtn, !connected && styles.disabled]}
          onPress={handleCapture}
          disabled={!connected}
        >
          <Text style={styles.buttonText}>
            {capturing ? 'CAPTURING...' : 'CAPTURE TRACE'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.button, styles.gainBtn]}
          onPress={handleAutoGain}
        >
          <Text style={styles.buttonText}>AUTO GAIN</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.paramRow}>
        <Text style={styles.label}>Set Gain:</Text>
        {[0, 12, 24, 36, 48].map((g) => (
          <TouchableOpacity
            key={g}
            style={[styles.gainPreset, gainDb === g && styles.gainPresetActive]}
            onPress={() => { setGainDb(g); sendCommand({ cmd: 'set_gain', gain_db: g }); }}
          >
            <Text style={styles.gainPresetText}>{g}dB</Text>
          </TouchableOpacity>
        ))}
      </View>

      {renderTracePreview()}

      <View style={styles.infoBox}>
        <Text style={styles.infoTitle}>Magnetic SCA Info</Text>
        <Text style={styles.infoText}>
          • DRV425 fluxgate sensor captures magnetic emanations{'\n'}
          • AD8331 VGA amplifies (0–48 dB programmable){'\n'}
          • LTC1564 LPF anti-aliases before ADC{'\n'}
          • AD7606 ADC: 16-bit, 800 kSPS, 6 channels{'\n'}
          • Trace stored in external 1 MB SRAM{'\n'}
          • Auto-gain finds optimal VGA setting
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#e74c3c', marginTop: 16, marginBottom: 12 },
  paramRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginBottom: 12 },
  label: { fontSize: 14, color: '#bbb' },
  input: { backgroundColor: '#1a1a2e', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, paddingHorizontal: 10, paddingVertical: 6, width: 100, textAlign: 'center' },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 16 },
  button: { flex: 1, paddingVertical: 14, borderRadius: 6, marginHorizontal: 4, alignItems: 'center' },
  captureBtn: { backgroundColor: '#2980b9' },
  gainBtn: { backgroundColor: '#27ae60' },
  disabled: { opacity: 0.4 },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  gainPreset: { backgroundColor: '#1a1a2e', paddingHorizontal: 8, paddingVertical: 6,
                borderRadius: 4, marginHorizontal: 2, borderWidth: 1, borderColor: '#333' },
  gainPresetActive: { backgroundColor: '#e74c3c', borderColor: '#e74c3c' },
  gainPresetText: { color: '#bbb', fontSize: 10, fontWeight: 'bold' },
  traceBox: { backgroundColor: '#111', borderRadius: 6, padding: 12, marginVertical: 12 },
  traceLabel: { color: '#95a5a6', fontSize: 12, marginBottom: 4 },
  traceContent: { color: '#2ecc71', fontSize: 10, fontFamily: 'monospace' },
  traceInfo: { color: '#555', fontSize: 10, marginTop: 4 },
  infoBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginTop: 16, marginBottom: 30 },
  infoTitle: { color: '#3498db', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  infoText: { color: '#aaa', fontSize: 12, lineHeight: 20 },
});