/**
 * eddy-phantom — RawCaptureScreen.js
 * Raw burst waveform viewer — displays 4-channel ADC data
 * from captured electromagnetic emanation bursts.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, SafeAreaView, StatusBar,
  TouchableOpacity, FlatList, PanResponder, Dimensions,
} from 'react-native';

const SCREEN_WIDTH = Dimensions.get('window').width;
const WAVEFORM_WIDTH = SCREEN_WIDTH - 32;
const WAVEFORM_HEIGHT = 60;

export default function RawCaptureScreen({ bleManager, connected, keystrokes }) {
  const [burstIndex, setBurstIndex] = useState(0);
  const [burstData, setBurstData] = useState(null);
  const [loading, setLoading] = useState(false);
  const [zoom, setZoom] = useState(1);
  const [selectedChannel, setSelectedChannel] = useState(0);

  // Request burst data from device
  const requestBurst = async (idx) => {
    if (!connected) return;
    setLoading(true);
    try {
      await bleManager.requestBurst(idx);
      // Data arrives via BLE notifications (onRawBurst callback)
      // For demo, we simulate the response
      setTimeout(() => {
        setBurstData(generateSimulatedBurst(idx));
        setLoading(false);
      }, 500);
    } catch (e) {
      setLoading(false);
    }
  };

  useEffect(() => {
    requestBurst(burstIndex);
  }, [burstIndex]);

  // Generate simulated waveform data for display
  function generateSimulatedBurst(idx) {
    const samples = 256;
    const channels = [[], [], [], []];
    for (let i = 0; i < samples; i++) {
      const t = i / samples;
      const envelope = Math.exp(-Math.pow((t - 0.3) * 5, 2)) +
                       Math.exp(-Math.pow((t - 0.6) * 8, 2)) * 0.6;
      channels[0].push(Math.sin(t * 80 * Math.PI) * envelope * 0.8 + (Math.random() - 0.5) * 0.1);
      channels[1].push(Math.sin(t * 60 * Math.PI) * envelope * 0.5 + (Math.random() - 0.5) * 0.08);
      channels[2].push(Math.sin(t * 100 * Math.PI) * envelope * 0.3 + (Math.random() - 0.5) * 0.05);
      channels[3].push(Math.sin(t * 40 * Math.PI) * envelope * 0.15 + (Math.random() - 0.5) * 0.03);
    }
    return { index: idx, samples, channels, timestamp: Date.now() };
  }

  // Render a waveform as text-based sparkline (React Native doesn't have canvas)
  function renderWaveform(channelData, channelIdx) {
    const maxVal = Math.max(...channelData.map(Math.abs), 0.001);
    const points = channelData.map((v, i) => {
      const x = (i / channelData.length) * WAVEFORM_WIDTH * zoom;
      const y = (v / maxVal) * (WAVEFORM_HEIGHT / 2) + WAVEFORM_HEIGHT / 2;
      return { x, y };
    });

    // Convert to SVG-like path string for visual representation
    const barWidth = WAVEFORM_WIDTH * zoom / channelData.length;
    return (
      <View style={styles.waveformContainer} key={channelIdx}>
        <View style={styles.waveformHeader}>
          <Text style={styles.channelLabel}>CH{channelIdx + 1}</Text>
          <View style={[styles.channelDot, {
            backgroundColor: CHANNEL_COLORS[channelIdx],
          }]} />
          <Text style={styles.ampLabel}>Peak: {(maxVal * 1000).toFixed(0)}mV</Text>
        </View>
        <View style={[styles.waveformBox, { height: WAVEFORM_HEIGHT }]}>
          {channelData.map((v, i) => {
            const h = Math.abs(v / maxVal) * (WAVEFORM_HEIGHT / 2);
            const isPositive = v >= 0;
            return (
              <View
                key={i}
                style={{
                  position: 'absolute',
                  left: (i / channelData.length) * WAVEFORM_WIDTH * zoom,
                  width: Math.max(barWidth, 1),
                  height: Math.max(h, 1),
                  backgroundColor: CHANNEL_COLORS[channelIdx],
                  opacity: 0.7,
                  top: isPositive ? WAVEFORM_HEIGHT / 2 - h : WAVEFORM_HEIGHT / 2,
                }}
              />
            );
          })}
          {/* Center line */}
          <View style={styles.centerLine} />
        </View>
      </View>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      {/* Burst selector */}
      <View style={styles.burstSelector}>
        <TouchableOpacity
          style={styles.navBtn}
          onPress={() => setBurstIndex(Math.max(0, burstIndex - 1))}
        >
          <Text style={styles.navBtnText}>◀ Prev</Text>
        </TouchableOpacity>
        <Text style={styles.burstIdx}>Burst #{burstIndex}</Text>
        <TouchableOpacity
          style={styles.navBtn}
          onPress={() => setBurstIndex(burstIndex + 1)}
        >
          <Text style={styles.navBtnText}>Next ▶</Text>
        </TouchableOpacity>
      </View>

      {/* Burst info */}
      {burstData && (
        <View style={styles.burstInfo}>
          <InfoItem label="Samples" value={burstData.samples.toString()} />
          <InfoItem label="Channels" value="4" />
          <InfoItem label="Sample Rate" value="1 MSPS" />
          <InfoItem label="Duration" value={`${(burstData.samples / 1000).toFixed(2)} ms`} />
        </View>
      )}

      {/* Zoom controls */}
      <View style={styles.zoomBar}>
        <Text style={styles.zoomLabel}>Zoom:</Text>
        {[0.5, 1, 2, 4].map(z => (
          <TouchableOpacity
            key={z}
            style={[styles.zoomBtn, zoom === z && styles.zoomBtnActive]}
            onPress={() => setZoom(z)}
          >
            <Text style={[styles.zoomBtnText, zoom === z && styles.zoomBtnTextActive]}>
              {z}x
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Waveforms */}
      {loading ? (
        <View style={styles.loadingContainer}>
          <Text style={styles.loadingText}>Loading burst data...</Text>
        </View>
      ) : burstData ? (
        <View style={styles.waveformsContainer}>
          <ScrollView horizontal style={styles.waveformScroll}>
            <View style={{ width: WAVEFORM_WIDTH * zoom }}>
              {burstData.channels.map((ch, idx) => renderWaveform(ch, idx))}
            </View>
          </ScrollView>
        </View>
      ) : (
        <Text style={styles.emptyText}>No burst data available</Text>
      )}

      {/* Feature info */}
      {burstData && (
        <View style={styles.featureCard}>
          <Text style={styles.featureTitle}>Extracted Features</Text>
          <Text style={styles.featureDesc}>
            MFCC (13) + Spatial (4) + Timing (2) = 19-dimensional vector
          </Text>
          <View style={styles.featureGrid}>
            {Array.from({ length: 19 }, (_, i) => (
              <View key={i} style={styles.featureCell}>
                <Text style={styles.featureIdx}>F{i}</Text>
                <View style={styles.featureBar}>
                  <View style={[styles.featureBarFill, {
                    width: `${20 + Math.random() * 70}%`,
                    backgroundColor: i < 13 ? '#4a90d9' : i < 17 ? '#e94560' : '#4ade80',
                  }]} />
                </View>
              </View>
            ))}
          </View>
        </View>
      )}
    </SafeAreaView>
  );
}

const CHANNEL_COLORS = ['#4a90d9', '#e94560', '#4ade80', '#f0a500'];

function InfoItem({ label, value }) {
  return (
    <View style={styles.infoItem}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

// ScrollView import workaround
import { ScrollView } from 'react-native';

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  burstSelector: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', padding: 12,
  },
  navBtn: {
    backgroundColor: '#1a1a2e', padding: 10, borderRadius: 8,
    borderWidth: 1, borderColor: '#2a2a3e',
  },
  navBtnText: { color: '#4a90d9', fontSize: 14 },
  burstIdx: { color: '#fff', fontSize: 18, fontWeight: 'bold' },
  burstInfo: {
    flexDirection: 'row', justifyContent: 'space-around',
    paddingHorizontal: 12, marginBottom: 12,
  },
  infoItem: { alignItems: 'center' },
  infoLabel: { color: '#555', fontSize: 10 },
  infoValue: { color: '#fff', fontSize: 14, fontWeight: '600', marginTop: 2 },
  zoomBar: {
    flexDirection: 'row', alignItems: 'center', paddingHorizontal: 12,
    gap: 6, marginBottom: 8,
  },
  zoomLabel: { color: '#888', fontSize: 12, marginRight: 4 },
  zoomBtn: { padding: 6, borderRadius: 4, borderWidth: 1, borderColor: '#2a2a3e' },
  zoomBtnActive: { borderColor: '#4a90d9', backgroundColor: '#4a90d922' },
  zoomBtnText: { color: '#666', fontSize: 11 },
  zoomBtnTextActive: { color: '#4a90d9' },
  waveformsContainer: { flex: 1, paddingHorizontal: 12 },
  waveformScroll: { flex: 1 },
  waveformContainer: { marginBottom: 12 },
  waveformHeader: { flexDirection: 'row', alignItems: 'center', marginBottom: 4 },
  channelLabel: { color: '#fff', fontSize: 12, fontWeight: 'bold', marginRight: 6 },
  channelDot: { width: 8, height: 8, borderRadius: 4, marginRight: 6 },
  ampLabel: { color: '#666', fontSize: 10, marginLeft: 'auto' },
  waveformBox: {
    backgroundColor: '#0a0a16', borderRadius: 6, overflow: 'hidden',
    borderWidth: 1, borderColor: '#2a2a3e', position: 'relative',
  },
  centerLine: {
    position: 'absolute', top: '50%', left: 0, right: 0,
    height: 1, backgroundColor: '#333',
  },
  loadingContainer: { flex: 1, justifyContent: 'center', alignItems: 'center' },
  loadingText: { color: '#666', fontSize: 14 },
  emptyText: { color: '#555', textAlign: 'center', marginTop: 40, fontSize: 14 },
  featureCard: {
    backgroundColor: '#1a1a2e', margin: 12, borderRadius: 12, padding: 16,
    borderWidth: 1, borderColor: '#2a2a3e',
  },
  featureTitle: { color: '#e94560', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  featureDesc: { color: '#666', fontSize: 10, marginBottom: 12 },
  featureGrid: { gap: 4 },
  featureCell: { flexDirection: 'row', alignItems: 'center' },
  featureIdx: { color: '#555', fontSize: 9, width: 30, fontFamily: 'monospace' },
  featureBar: {
    flex: 1, height: 4, backgroundColor: '#2a2a3e', borderRadius: 2, overflow: 'hidden',
  },
  featureBarFill: { height: '100%', borderRadius: 2 },
});