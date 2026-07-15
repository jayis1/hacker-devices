/**
 * TeslaPhantom — Trace Analysis Screen
 * View captured SCA traces, apply filters, compute correlation.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Dimensions } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const { width: screenWidth } = Dimensions.get('window');

export default function TraceAnalysisScreen() {
  const { traceData, sendCommand } = useDevice();
  const [filterMode, setFilterMode] = useState('raw');
  const [zoomLevel, setZoomLevel] = useState(1);

  // Decode trace data into sample array
  const getSamples = () => {
    if (!traceData || traceData.length < 4) return [];
    const samples = [];
    for (let i = 0; i + 1 < traceData.length; i += 2) {
      const val = traceData.charCodeAt(i) | (traceData.charCodeAt(i + 1) << 8);
      // Convert to signed 16-bit
      samples.push(val > 32767 ? val - 65536 : val);
    }
    return samples;
  };

  const samples = getSamples();

  // Render trace as SVG-like text waveform
  const renderWaveform = () => {
    if (samples.length === 0) return null;
    const maxPoints = Math.floor(screenWidth - 32);
    const step = Math.max(1, Math.floor(samples.length / maxPoints));
    const maxVal = 32768;

    // Build ASCII waveform
    const lines = 10;
    const grid = Array(lines).fill(null).map(() => []);

    for (let i = 0; i < samples.length; i += step) {
      let val = samples[i];
      // Apply filter based on mode
      if (filterMode === 'envelope') {
        val = Math.abs(val);
      } else if (filterMode === 'normalized') {
        // Normalize to local max
        const localMax = Math.max(...samples.slice(Math.max(0, i - 100), i + 100).map(Math.abs));
        val = localMax > 0 ? (val / localMax) * 32768 : 0;
      }

      const normalized = val / maxVal; // -1 to 1
      const row = Math.floor((1 - (normalized + 1) / 2) * (lines - 1));
      for (let r = 0; r < lines; r++) {
        grid[r].push(r === row ? '█' : (r === lines - 1 || r === 0) ? '─' : ' ');
      }
    }

    return (
      <View style={styles.waveformBox}>
        {grid.map((row, idx) => (
          <Text key={idx} style={styles.waveformLine} numberOfLines={1}>
            {row.join('')}
          </Text>
        ))}
      </View>
    );
  };

  // Compute basic statistics
  const computeStats = () => {
    if (samples.length === 0) return null;
    let sum = 0, sumSq = 0, peak = 0;
    for (const s of samples) {
      sum += s;
      sumSq += s * s;
      const abs = Math.abs(s);
      if (abs > peak) peak = abs;
    }
    const mean = sum / samples.length;
    const variance = sumSq / samples.length - mean * mean;
    const rms = Math.sqrt(Math.max(0, variance));
    return { mean, rms, peak, count: samples.length };
  };

  const stats = computeStats();

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Trace Analysis</Text>

      {!traceData || samples.length === 0 ? (
        <View style={styles.emptyBox}>
          <Text style={styles.emptyText}>No trace data</Text>
          <Text style={styles.emptyHint}>Capture a trace from the SCA tab</Text>
        </View>
      ) : (
        <>
          <View style={styles.filterRow}>
            {['raw', 'envelope', 'normalized'].map((mode) => (
              <TouchableOpacity
                key={mode}
                style={[styles.filterBtn, filterMode === mode && styles.filterBtnActive]}
                onPress={() => setFilterMode(mode)}
              >
                <Text style={[styles.filterBtnText, filterMode === mode && styles.filterBtnTextActive]}>
                  {mode.toUpperCase()}
                </Text>
              </TouchableOpacity>
            ))}
          </View>

          <View style={styles.zoomRow}>
            <Text style={styles.label}>Zoom: {zoomLevel}x</Text>
            <TouchableOpacity style={styles.zoomBtn} onPress={() => setZoomLevel(Math.max(1, zoomLevel - 1))}>
              <Text style={styles.zoomBtnText}>-</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.zoomBtn} onPress={() => setZoomLevel(Math.min(10, zoomLevel + 1))}>
              <Text style={styles.zoomBtnText}>+</Text>
            </TouchableOpacity>
          </View>

          {renderWaveform()}

          {stats && (
            <View style={styles.statsBox}>
              <Text style={styles.statsTitle}>Trace Statistics</Text>
              <View style={styles.statsRow}>
                <Text style={styles.statsLabel}>Samples:</Text>
                <Text style={styles.statsValue}>{stats.count}</Text>
              </View>
              <View style={styles.statsRow}>
                <Text style={styles.statsLabel}>Mean:</Text>
                <Text style={styles.statsValue}>{stats.mean.toFixed(2)}</Text>
              </View>
              <View style={styles.statsRow}>
                <Text style={styles.statsLabel}>RMS:</Text>
                <Text style={styles.statsValue}>{stats.rms.toFixed(2)}</Text>
              </View>
              <View style={styles.statsRow}>
                <Text style={styles.statsLabel}>Peak:</Text>
                <Text style={styles.statsValue}>{stats.peak}</Text>
              </View>
              <View style={styles.statsRow}>
                <Text style={styles.statsLabel}>SNR est:</Text>
                <Text style={styles.statsValue}>
                  {stats.rms > 0 ? (20 * Math.log10(stats.peak / stats.rms)).toFixed(1) : 'N/A'} dB
                </Text>
              </View>
            </View>
          )}

          <View style={styles.analysisBox}>
            <Text style={styles.analysisTitle}>Analysis Notes</Text>
            <Text style={styles.analysisText}>
              • Peak amplitude indicates strong magnetic emanation{'\n'}
              • High SNR suggests exploitable leakage{'\n'}
              • Use envelope mode to visualize energy distribution{'\n'}
              • Correlate with known crypto operations for CPA{'\n'}
              • Look for data-dependent amplitude variations
            </Text>
          </View>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#e74c3c', marginTop: 16, marginBottom: 12 },
  emptyBox: { alignItems: 'center', paddingVertical: 60 },
  emptyText: { color: '#95a5a6', fontSize: 16 },
  emptyHint: { color: '#555', fontSize: 12, marginTop: 8 },
  filterRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  filterBtn: { flex: 1, backgroundColor: '#1a1a2e', paddingVertical: 10, marginHorizontal: 2,
               borderRadius: 4, borderWidth: 1, borderColor: '#333' },
  filterBtnActive: { backgroundColor: '#2980b9', borderColor: '#2980b9' },
  filterBtnText: { color: '#95a5a6', fontSize: 11, textAlign: 'center', fontWeight: 'bold' },
  filterBtnTextActive: { color: '#fff' },
  zoomRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  label: { color: '#bbb', fontSize: 14, marginRight: 12 },
  zoomBtn: { backgroundColor: '#1a1a2e', width: 32, height: 32, borderRadius: 4,
             justifyContent: 'center', alignItems: 'center', marginHorizontal: 4 },
  zoomBtnText: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  waveformBox: { backgroundColor: '#000', borderRadius: 4, padding: 8, marginVertical: 8 },
  waveformLine: { color: '#2ecc71', fontSize: 7, fontFamily: 'monospace', lineHeight: 8 },
  statsBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginVertical: 12 },
  statsTitle: { color: '#27ae60', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  statsRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 3 },
  statsLabel: { color: '#95a5a6', fontSize: 13 },
  statsValue: { color: '#fff', fontSize: 13, fontWeight: 'bold' },
  analysisBox: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 12, marginBottom: 30 },
  analysisTitle: { color: '#3498db', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  analysisText: { color: '#aaa', fontSize: 12, lineHeight: 20 },
});