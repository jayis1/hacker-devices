//==============================================================================
// TempestScreen.tsx — Tempest Decode View
// Author: jayis1
// Description: Displays reconstructed video frames from EM leakage captured
//              by the Spectre-Sniffer device.
//==============================================================================

import React, { useEffect, useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Image,
  TouchableOpacity,
  ActivityIndicator,
  ScrollView,
} from 'react-native';
import { SpectreAPI } from '../services/api';
import { TempestFrame } from '../types';

const TempestScreen: React.FC = () => {
  const [frame, setFrame] = useState<TempestFrame | null>(null);
  const [loading, setLoading] = useState(false);
  const [active, setActive] = useState(false);
  const [pixelClock, setPixelClock] = useState('74250000');
  const [imageUri, setImageUri] = useState<string | null>(null);

  const startTempest = useCallback(async () => {
    setLoading(true);
    try {
      await SpectreAPI.startTempest(parseInt(pixelClock, 10));
      setActive(true);
    } catch (err) {
      console.error('Failed to start Tempest:', err);
    }
    setLoading(false);
  }, [pixelClock]);

  const stopTempest = useCallback(async () => {
    try {
      await SpectreAPI.stopTempest();
      setActive(false);
    } catch (err) {
      console.error('Failed to stop Tempest:', err);
    }
  }, []);

  useEffect(() => {
    if (!active) return;

    const interval = setInterval(async () => {
      try {
        const data = await SpectreAPI.getTempestImage();
        setFrame(data);
        // Convert pixel data to data URI for Image component
        // (simplified - actual implementation would use base64 encoding)
      } catch (err) {
        // Ignore polling errors
      }
    }, 200);

    return () => clearInterval(interval);
  }, [active]);

  return (
    <ScrollView style={styles.container}>
      {/* Controls */}
      <View style={styles.controls}>
        <Text style={styles.sectionTitle}>Tempest Decoder</Text>
        <Text style={styles.description}>
          Reconstruct video display content from unintentional electromagnetic
          emanations (van Eck phreaking).
        </Text>

        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Pixel Clock (Hz):</Text>
          <TouchableOpacity
            style={styles.input}
            onPress={() => {
              const presets = ['25175000', '40000000', '65000000', '74250000', '108000000', '148500000'];
              const idx = presets.indexOf(pixelClock);
              setPixelClock(presets[(idx + 1) % presets.length]);
            }}
          >
            <Text style={styles.inputText}>{parseInt(pixelClock) / 1e6} MHz</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.buttonRow}>
          {!active ? (
            <TouchableOpacity
              style={[styles.button, styles.startButton]}
              onPress={startTempest}
              disabled={loading}
            >
              {loading ? (
                <ActivityIndicator color="#fff" />
              ) : (
                <Text style={styles.buttonText}>Start Tempest</Text>
              )}
            </TouchableOpacity>
          ) : (
            <TouchableOpacity
              style={[styles.button, styles.stopButton]}
              onPress={stopTempest}
            >
              <Text style={styles.buttonText}>Stop Tempest</Text>
            </TouchableOpacity>
          )}
        </View>
      </View>

      {/* Status */}
      {active && (
        <View style={styles.statusCard}>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Status:</Text>
            <Text style={[styles.statusValue, { color: '#00E676' }]}>Active</Text>
          </View>
          {frame && (
            <>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Resolution:</Text>
                <Text style={styles.statusValue}>
                  {frame.width} x {frame.height}
                </Text>
              </View>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Signal Quality:</Text>
                <Text
                  style={[
                    styles.statusValue,
                    {
                      color:
                        frame.signalQuality > 0.7
                          ? '#00E676'
                          : frame.signalQuality > 0.4
                          ? '#FFC107'
                          : '#FF5252',
                    },
                  ]}
                >
                  {(frame.signalQuality * 100).toFixed(0)}%
                </Text>
              </View>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Video Mode:</Text>
                <Text style={styles.statusValue}>{frame.videoMode}</Text>
              </View>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Frame:</Text>
                <Text style={styles.statusValue}>#{frame.frameNumber}</Text>
              </View>
            </>
          )}
        </View>
      )}

      {/* Reconstructed Image */}
      {frame && (
        <View style={styles.imageContainer}>
          <Text style={styles.sectionTitle}>Reconstructed Display</Text>
          <View style={styles.imagePlaceholder}>
            <Text style={styles.imagePlaceholderText}>
              {frame.width} x {frame.height} px
            </Text>
            <Text style={styles.imagePlaceholderSubtext}>
              Signal Quality: {(frame.signalQuality * 100).toFixed(0)}%
            </Text>
            {/* In production, render pixel data as an Image */}
            <View style={styles.pixelPreview}>
              {Array.from({ length: Math.min(frame.height, 20) }).map((_, y) => (
                <View key={y} style={{ flexDirection: 'row' }}>
                  {Array.from({ length: Math.min(frame.width, 40) }).map((_, x) => {
                    const idx = (y * frame.width + x) * 4;
                    const r = frame.pixelData[idx] || 0;
                    const g = frame.pixelData[idx + 1] || 0;
                    const b = frame.pixelData[idx + 2] || 0;
                    return (
                      <View
                        key={x}
                        style={{
                          width: 4,
                          height: 4,
                          backgroundColor: `rgb(${r},${g},${b})`,
                        }}
                      />
                    );
                  })}
                </View>
              ))}
            </View>
          </View>
        </View>
      )}

      {/* Preset Video Modes */}
      <View style={styles.presets}>
        <Text style={styles.sectionTitle}>Common Video Modes</Text>
        {[
          { name: '640x480 @ 60Hz', clock: '25175000' },
          { name: '800x600 @ 60Hz', clock: '40000000' },
          { name: '1024x768 @ 60Hz', clock: '65000000' },
          { name: '1280x720 @ 60Hz', clock: '74250000' },
          { name: '1280x1024 @ 60Hz', clock: '108000000' },
          { name: '1920x1080 @ 60Hz', clock: '148500000' },
        ].map((mode) => (
          <TouchableOpacity
            key={mode.clock}
            style={styles.presetItem}
            onPress={() => setPixelClock(mode.clock)}
          >
            <Text style={styles.presetName}>{mode.name}</Text>
            <Text style={styles.presetClock}>
              {(parseInt(mode.clock) / 1e6).toFixed(1)} MHz
            </Text>
          </TouchableOpacity>
        ))}
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  controls: {
    marginBottom: 24,
  },
  sectionTitle: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  description: {
    color: '#888',
    fontSize: 14,
    lineHeight: 20,
    marginBottom: 16,
  },
  inputRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  inputLabel: {
    color: '#aaa',
    fontSize: 14,
    marginRight: 12,
  },
  input: {
    backgroundColor: '#1a1a2e',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  inputText: {
    color: '#00E676',
    fontSize: 16,
    fontWeight: 'bold',
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'center',
  },
  button: {
    paddingHorizontal: 32,
    paddingVertical: 12,
    borderRadius: 8,
    minWidth: 160,
    alignItems: 'center',
  },
  startButton: {
    backgroundColor: '#00E676',
  },
  stopButton: {
    backgroundColor: '#FF5252',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  statusCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 24,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  statusLabel: {
    color: '#888',
    fontSize: 14,
  },
  statusValue: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  imageContainer: {
    marginBottom: 24,
  },
  imagePlaceholder: {
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    padding: 16,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#2a2a4e',
    minHeight: 200,
    justifyContent: 'center',
  },
  imagePlaceholderText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  imagePlaceholderSubtext: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
  },
  pixelPreview: {
    marginTop: 16,
    borderWidth: 1,
    borderColor: '#333',
  },
  presets: {
    marginBottom: 24,
  },
  presetItem: {
    backgroundColor: '#1a1a2e',
    flexDirection: 'row',
    justifyContent: 'space-between',
    padding: 12,
    borderRadius: 8,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  presetName: {
    color: '#fff',
    fontSize: 14,
  },
  presetClock: {
    color: '#448AFF',
    fontSize: 14,
    fontWeight: '600',
  },
});

export default TempestScreen;
