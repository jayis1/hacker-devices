/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Capture Screen — Spectrum waterfall, live capture, storage management
 *
 * Shows a real-time spectrum waterfall of the ultrasonic environment,
 * manages capture sessions, and interfaces with device storage.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
  ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import {
  buildSetMode,
  buildGetStatus,
  MODE,
  MODE_NAMES,
} from '../utils/protocol';
import FreqChart from '../components/FreqChart';

const SERVICE_UUID = '00001810-0000-1000-8000-00805f9b34fb';
const CHARACTERISTIC_UUID = '00002a35-0000-1000-8000-00805f9b34fb';

const SPECTRUM_BINS = 256;
const SPECTRUM_FREQ_MIN = 18000;
const SPECTRUM_FREQ_MAX = 45000;

/**
 * Capture screen — live spectrum and capture management.
 */
export default function CaptureScreen({ bleManager, connectedDevice }) {
  const [isCapturing, setIsCapturing] = useState(false);
  const [spectrumData, setSpectrumData] = useState(
    new Array(SPECTRUM_BINS).fill(0),
  );
  const [history, setHistory] = useState([]); // Array of spectrum snapshots
  const [storageSlots, setStorageSlots] = useState(0);
  const [detailedSpectrum, setDetailedSpectrum] = useState(false);
  const pollingRef = useRef(null);

  // Start capture (starts Rx mode + spectrum polling)
  const startCapture = useCallback(async () => {
    if (!connectedDevice) {
      Alert.alert('Not Connected', 'Connect to a device first.');
      return;
    }

    setIsCapturing(true);

    // Set device to Rx FSK mode
    try {
      await connectedDevice.writeCharacteristicWithResponseForService(
        SERVICE_UUID,
        CHARACTERISTIC_UUID,
        Buffer.from(buildSetMode(MODE.RX_FSK)).toString('base64'),
      );
    } catch (e) {
      Alert.alert('Error', 'Failed to start capture mode');
      setIsCapturing(false);
      return;
    }

    // Poll spectrum every 500ms
    pollingRef.current = setInterval(async () => {
      try {
        // Request spectrum data via RX_SPECTRUM command
        // (In production, the firmware would push this via notification)
        // For now, simulate spectrum data
        simulateSpectrum();
      } catch (e) {
        // Device may be gone
      }
    }, 500);
  }, [connectedDevice]);

  // Stop capture
  const stopCapture = useCallback(async () => {
    setIsCapturing(false);
    if (pollingRef.current) {
      clearInterval(pollingRef.current);
      pollingRef.current = null;
    }

    if (connectedDevice) {
      try {
        await connectedDevice.writeCharacteristicWithResponseForService(
          SERVICE_UUID,
          CHARACTERISTIC_UUID,
          Buffer.from(buildSetMode(MODE.IDLE)).toString('base64'),
        );
      } catch (e) {
        // ignore
      }
    }
  }, [connectedDevice]);

  // Simulate spectrum data for development
  const simulateSpectrum = useCallback(() => {
    const newData = new Array(SPECTRUM_BINS).fill(0);

    // Add some noise floor
    for (let i = 0; i < SPECTRUM_BINS; i++) {
      newData[i] = Math.random() * 0.1; // Noise floor
    }

    // Simulate a signal at ~21 kHz (FSK mark)
    const markBin = Math.floor(((21000 - SPECTRUM_FREQ_MIN) / (SPECTRUM_FREQ_MAX - SPECTRUM_FREQ_MIN)) * SPECTRUM_BINS);
    for (let i = -3; i <= 3; i++) {
      const bin = markBin + i;
      if (bin >= 0 && bin < SPECTRUM_BINS) {
        newData[bin] += 0.6 - Math.abs(i) * 0.15;
      }
    }

    // Simulate a weaker signal at ~19.5 kHz (FSK space)
    const spaceBin = Math.floor(((19500 - SPECTRUM_FREQ_MIN) / (SPECTRUM_FREQ_MAX - SPECTRUM_FREQ_MIN)) * SPECTRUM_BINS);
    for (let i = -2; i <= 2; i++) {
      const bin = spaceBin + i;
      if (bin >= 0 && bin < SPECTRUM_BINS) {
        newData[bin] += 0.3 - Math.abs(i) * 0.1;
      }
    }

    setSpectrumData(newData);

    // Add to history (keep last 50 frames for waterfall)
    setHistory((prev) => {
      const h = [...prev, [...newData]];
      if (h.length > 50) h.shift();
      return h;
    });
  }, []);

  // Cleanup
  useEffect(() => {
    return () => {
      if (pollingRef.current) clearInterval(pollingRef.current);
    };
  }, []);

  // Find dominant frequency
  const dominantFreq = () => {
    let maxVal = 0;
    let maxIdx = 0;
    for (let i = 0; i < spectrumData.length; i++) {
      if (spectrumData[i] > maxVal) {
        maxVal = spectrumData[i];
        maxIdx = i;
      }
    }
    const freq = SPECTRUM_FREQ_MIN + (maxIdx / SPECTRUM_BINS) * (SPECTRUM_FREQ_MAX - SPECTRUM_FREQ_MIN);
    return { freq: Math.round(freq), level: maxVal };
  };

  const dom = dominantFreq();

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Spectrum Display */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Ultrasonic Spectrum</Text>
        <Text style={styles.freqRange}>
          18 kHz – 45 kHz · {isCapturing ? 'Live' : 'Paused'}
        </Text>

        <View style={styles.chartContainer}>
          <FreqChart
            data={spectrumData}
            width={320}
            height={160}
            minFreq={SPECTRUM_FREQ_MIN}
            maxFreq={SPECTRUM_FREQ_MAX}
          />
        </View>

        {/* Dominant frequency */}
        {dom.level > 0.15 && (
          <View style={styles.dominantBox}>
            <Text style={styles.dominantLabel}>Dominant Signal</Text>
            <Text style={styles.dominantValue}>
              {dom.freq} Hz
              {dom.freq >= 18000 && dom.freq <= 20000
                ? ' (Space)'
                : dom.freq >= 20500 && dom.freq <= 21500
                ? ' (Mark)'
                : ''}
            </Text>
            <Text style={styles.dominantLevel}>
              Level: {(dom.level * 100).toFixed(0)}%
            </Text>
          </View>
        )}
      </View>

      {/* Signal Quality */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Signal Analysis</Text>
        <View style={styles.signalRow}>
          <Text style={styles.signalLabel}>Mark (21.0 kHz):</Text>
          <View style={styles.signalBar}>
            <View
              style={[
                styles.signalFill,
                {
                  width: `${Math.min(spectrumData[Math.floor(((21000 - SPECTRUM_FREQ_MIN) / (SPECTRUM_FREQ_MAX - SPECTRUM_FREQ_MIN)) * SPECTRUM_BINS)] * 100, 100)}%`,
                  backgroundColor: '#00d4ff',
                },
              ]}
            />
          </View>
        </View>
        <View style={styles.signalRow}>
          <Text style={styles.signalLabel}>Space (19.5 kHz):</Text>
          <View style={styles.signalBar}>
            <View
              style={[
                styles.signalFill,
                {
                  width: `${Math.min(spectrumData[Math.floor(((19500 - SPECTRUM_FREQ_MIN) / (SPECTRUM_FREQ_MAX - SPECTRUM_FREQ_MIN)) * SPECTRUM_BINS)] * 100, 100)}%`,
                  backgroundColor: '#ffaa00',
                },
              ]}
            />
          </View>
        </View>
        <Text style={styles.snrText}>
          Estimated SNR:{' '}
          {dom.level > 0.2
            ? `${(20 * Math.log10(dom.level / 0.05)).toFixed(1)} dB`
            : 'No signal detected'}
        </Text>
      </View>

      {/* Controls */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Capture Controls</Text>
        <View style={styles.controlRow}>
          <TouchableOpacity
            style={[
              styles.controlBtn,
              isCapturing ? styles.controlBtnActive : null,
            ]}
            onPress={isCapturing ? stopCapture : startCapture}
            disabled={!connectedDevice}
          >
            <Icon
              name={isCapturing ? 'stop' : 'mic'}
              size={24}
              color="#fff"
            />
            <Text style={styles.controlText}>
              {isCapturing ? 'Stop Capture' : 'Start Capture'}
            </Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={styles.controlBtn}
            onPress={() => setDetailedSpectrum(!detailedSpectrum)}
          >
            <Icon
              name={detailedSpectrum ? 'visibility' : 'visibility-off'}
              size={20}
              color="#fff"
            />
            <Text style={styles.controlText}>
              {detailedSpectrum ? 'Simple' : 'Detailed'}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Storage Info */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Storage</Text>
        <Text style={styles.storageText}>
          {storageSlots} capture slots used
        </Text>
        <TouchableOpacity
          style={styles.storageBtn}
          onPress={() => {
            /* Request status to get storage info */
            if (connectedDevice) {
              connectedDevice
                .writeCharacteristicWithResponseForService(
                  SERVICE_UUID,
                  CHARACTERISTIC_UUID,
                  Buffer.from(buildGetStatus()).toString('base64'),
                )
                .catch((e) => console.warn(e));
            }
          }}
        >
          <Text style={styles.storageBtnText}>Refresh Storage Info</Text>
        </TouchableOpacity>
      </View>

      {/* Waterfall History */}
      {history.length > 0 && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Spectrum History (Waterfall)</Text>
          <Text style={styles.waterfallInfo}>
            Last {history.length} frames · Newest at top
          </Text>
          <View style={styles.waterfallContainer}>
            {history
              .slice()
              .reverse()
              .slice(0, 20)
              .map((frame, idx) => (
                <View key={idx} style={styles.waterfallRow}>
                  {frame.map((val, bin) => (
                    <View
                      key={bin}
                      style={{
                        flex: 1,
                        height: 3,
                        backgroundColor: `rgb(${Math.min(
                          val * 255 * 4,
                          255,
                        )}, ${Math.min(val * 128, 128)}, ${Math.max(
                          255 - val * 255 * 4,
                          0,
                        )})`,
                      }}
                    />
                  ))}
                </View>
              ))}
          </View>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#16213e',
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
    marginBottom: 4,
  },
  freqRange: {
    color: '#6c757d',
    fontSize: 11,
    marginBottom: 12,
  },
  chartContainer: {
    alignItems: 'center',
    marginBottom: 8,
  },
  dominantBox: {
    backgroundColor: '#0d1a1a',
    borderRadius: 8,
    padding: 10,
    borderWidth: 1,
    borderColor: '#0d7377',
    marginTop: 8,
  },
  dominantLabel: {
    color: '#00d4ff',
    fontSize: 11,
    fontWeight: '600',
  },
  dominantValue: {
    color: '#e0e0e0',
    fontSize: 18,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  dominantLevel: {
    color: '#6c757d',
    fontSize: 11,
  },
  signalRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
    gap: 8,
  },
  signalLabel: {
    color: '#b0b0b0',
    fontSize: 12,
    width: 110,
  },
  signalBar: {
    flex: 1,
    height: 12,
    backgroundColor: '#16213e',
    borderRadius: 6,
    overflow: 'hidden',
  },
  signalFill: {
    height: '100%',
    borderRadius: 6,
  },
  snrText: {
    color: '#6c757d',
    fontSize: 11,
    fontFamily: 'monospace',
    marginTop: 4,
  },
  controlRow: {
    flexDirection: 'row',
    gap: 8,
  },
  controlBtn: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    gap: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  controlBtnActive: {
    backgroundColor: '#441111',
    borderColor: '#ff4444',
  },
  controlText: {
    color: '#fff',
    fontSize: 13,
    fontWeight: '600',
  },
  storageText: {
    color: '#b0b0b0',
    fontSize: 13,
    marginBottom: 8,
  },
  storageBtn: {
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  storageBtnText: {
    color: '#00d4ff',
    fontSize: 12,
  },
  waterfallInfo: {
    color: '#6c757d',
    fontSize: 10,
    marginBottom: 8,
  },
  waterfallContainer: {
    borderRadius: 8,
    overflow: 'hidden',
  },
  waterfallRow: {
    flexDirection: 'row',
    height: 3,
  },
});