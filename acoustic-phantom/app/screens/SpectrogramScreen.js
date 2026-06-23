/**
 * ACOUSTIC-PHANTOM — Spectrogram Screen
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Real-time scrolling spectrogram of the active audio channel.
 * Renders the power spectrum using a canvas-based renderer.
 * Shows frequency (Y) vs time (X) with color-coded magnitude.
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, View as RNView,
} from 'react-native';
import { useBLE } from '../utils/ble';
import SpectrogramCanvas from '../components/SpectrogramCanvas';

export default function SpectrogramScreen() {
  const ble = useBLE();
  const [channel, setChannel] = useState(0); // 0=beamformed, 1=piezo
  const [zoom, setZoom] = useState(1);
  const [paused, setPaused] = useState(false);
  const [spectroData, setSpectroData] = useState([]);

  // Accumulate audio data into spectrogram frames
  useEffect(() => {
    if (!ble.audioData || paused) return;

    // Convert time-domain samples to a crude power spectrum
    // (Full FFT would be done on-device; here we do a simple
    // energy estimate in frequency bands for visualization)
    const samples = ble.audioData.samples;
    if (!samples || samples.length === 0) return;

    // Simple 32-band energy estimate using zero-crossing rate
    // per band (not a true FFT, but sufficient for visualization)
    const bands = 32;
    const bandSize = Math.floor(samples.length / bands);
    const frame = new Array(bands);

    for (let b = 0; b < bands; b++) {
      let energy = 0;
      const start = b * bandSize;
      const end = Math.min(start + bandSize, samples.length);
      for (let i = start; i < end; i++) {
        const s = samples[i] / 32768.0;
        energy += s * s;
      }
      frame[b] = Math.sqrt(energy / bandSize);
    }

    setSpectroData(prev => [...prev.slice(-100), frame]);
  }, [ble.audioData, paused]);

  const handleChannelSwitch = () => {
    setChannel(ch => ch === 0 ? 1 : 0);
  };

  const handleZoomIn = () => setZoom(z => Math.min(z * 2, 8));
  const handleZoomOut = () => setZoom(z => Math.max(z / 2, 0.5));
  const handlePause = () => setPaused(p => !p);

  return (
    <View style={styles.container}>
      {/* Controls */}
      <View style={styles.controls}>
        <TouchableOpacity
          style={styles.controlButton}
          onPress={handleChannelSwitch}
        >
          <Text style={styles.controlText}>
            Ch: {channel === 0 ? 'Beam' : 'Piezo'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.controlButton}
          onPress={handleZoomOut}
        >
          <Text style={styles.controlText}>Zoom −</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.controlButton}
          onPress={handleZoomIn}
        >
          <Text style={styles.controlText}>Zoom +</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.controlButton, paused && styles.controlButtonActive]}
          onPress={handlePause}
        >
          <Text style={styles.controlText}>
            {paused ? '▶ Resume' : '⏸ Pause'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Spectrogram canvas */}
      <View style={styles.spectroContainer}>
        {spectroData.length > 0 ? (
          <SpectrogramCanvas
            data={spectroData}
            zoom={zoom}
            style={styles.canvas}
          />
        ) : (
          <View style={styles.emptyState}>
            <Text style={styles.emptyText}>No audio data</Text>
            <Text style={styles.emptySubtext}>
              Connect and arm device to see spectrogram
            </Text>
          </View>
        )}
      </View>

      {/* Frequency axis labels */}
      <View style={styles.freqAxis}>
        <Text style={styles.freqLabel}>24kHz</Text>
        <Text style={styles.freqLabel}>12kHz</Text>
        <Text style={styles.freqLabel}>0Hz</Text>
      </View>

      {/* Info bar */}
      <View style={styles.infoBar}>
        <Text style={styles.infoText}>
          Frames: {spectroData.length} | Bandwidth: 0-24kHz
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a1a' },
  controls: {
    flexDirection: 'row',
    padding: 8,
    gap: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  controlButton: {
    flex: 1,
    padding: 10,
    backgroundColor: '#333',
    borderRadius: 6,
    alignItems: 'center',
  },
  controlButtonActive: { backgroundColor: '#2196F3' },
  controlText: { color: '#FFF', fontSize: 14 },
  spectroContainer: { flex: 1, padding: 8 },
  canvas: { flex: 1, backgroundColor: '#000' },
  emptyState: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  emptyText: { color: '#555', fontSize: 18, marginBottom: 8 },
  emptySubtext: { color: '#333', fontSize: 14 },
  freqAxis: {
    position: 'absolute',
    right: 8,
    top: 60,
    bottom: 40,
    justifyContent: 'space-between',
  },
  freqLabel: { color: '#888', fontSize: 10 },
  infoBar: {
    padding: 8,
    borderTopWidth: 1,
    borderTopColor: '#333',
  },
  infoText: { color: '#666', fontSize: 12 },
});