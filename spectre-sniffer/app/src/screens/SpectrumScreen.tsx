//==============================================================================
// SpectrumScreen.tsx — Real-time Spectrum Analyzer View
// Author: jayis1
// Description: Displays real-time FFT spectrum and waterfall from the
//              Spectre-Sniffer device over WebSocket connection.
//==============================================================================

import React, { useEffect, useState, useRef, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Dimensions,
  TouchableOpacity,
  ActivityIndicator,
} from 'react-native';
import { SpectreAPI } from '../services/api';
import { SpectrumData } from '../types';

const SCREEN_WIDTH = Dimensions.get('window').width;
const SPECTRUM_HEIGHT = 200;
const WATERFALL_HEIGHT = 200;

const SpectrumScreen: React.FC = () => {
  const [spectrum, setSpectrum] = useState<SpectrumData | null>(null);
  const [connected, setConnected] = useState(false);
  const [peakHold, setPeakHold] = useState<number[]>([]);
  const [waterfall, setWaterfall] = useState<number[][]>([]);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    // Connect to WebSocket for real-time spectrum data
    const connectWs = () => {
      const ws = new WebSocket(`ws://${SpectreAPI.getBaseUrl().replace('http://', '')}/api/spectrum/stream`);

      ws.onopen = () => {
        setConnected(true);
      };

      ws.onmessage = (event) => {
        try {
          const data: SpectrumData = JSON.parse(event.data);
          setSpectrum(data);

          // Update peak hold
          setPeakHold((prev) => {
            const newPeaks = data.magnitudes.map((mag, i) =>
              Math.max(mag, prev[i] || 0)
            );
            return newPeaks;
          });

          // Update waterfall
          setWaterfall((prev) => {
            const newWaterfall = [...prev, data.magnitudes];
            if (newWaterfall.length > 100) {
              return newWaterfall.slice(-100);
            }
            return newWaterfall;
          });
        } catch (e) {
          // Ignore parse errors
        }
      };

      ws.onclose = () => {
        setConnected(false);
        // Reconnect after 2 seconds
        setTimeout(connectWs, 2000);
      };

      ws.onerror = () => {
        ws.close();
      };

      wsRef.current = ws;
    };

    connectWs();

    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, []);

  // Render spectrum bars
  const renderSpectrum = () => {
    if (!spectrum) return null;

    const { magnitudes, maxMagnitude } = spectrum;
    const barWidth = (SCREEN_WIDTH - 32) / magnitudes.length;

    return (
      <View style={styles.spectrumContainer}>
        <Text style={styles.sectionTitle}>Real-Time Spectrum</Text>
        <View style={styles.spectrumGraph}>
          {magnitudes.map((mag, i) => {
            const height = (mag / maxMagnitude) * SPECTRUM_HEIGHT;
            const peakHeight = (peakHold[i] / maxMagnitude) * SPECTRUM_HEIGHT;
            return (
              <View key={i} style={{ width: barWidth, height: SPECTRUM_HEIGHT, justifyContent: 'flex-end' }}>
                {/* Peak hold line */}
                <View
                  style={{
                    position: 'absolute',
                    bottom: peakHeight,
                    width: barWidth,
                    height: 2,
                    backgroundColor: '#FFC107',
                  }}
                />
                {/* Current magnitude bar */}
                <View
                  style={{
                    height: Math.max(height, 1),
                    backgroundColor: getColorForMagnitude(mag, maxMagnitude),
                    width: barWidth - 1,
                  }}
                />
              </View>
            );
          })}
        </View>
        {/* Frequency labels */}
        <View style={styles.freqLabels}>
          <Text style={styles.freqLabel}>
            {(spectrum.centerFrequencyHz - spectrum.spanHz / 2) / 1e6} MHz
          </Text>
          <Text style={styles.freqLabel}>
            {spectrum.centerFrequencyHz / 1e6} MHz
          </Text>
          <Text style={styles.freqLabel}>
            {(spectrum.centerFrequencyHz + spectrum.spanHz / 2) / 1e6} MHz
          </Text>
        </View>
      </View>
    );
  };

  // Render waterfall
  const renderWaterfall = () => {
    if (waterfall.length === 0) return null;

    const binWidth = (SCREEN_WIDTH - 32) / (waterfall[0]?.length || 1);

    return (
      <View style={styles.waterfallContainer}>
        <Text style={styles.sectionTitle}>Waterfall</Text>
        <View style={styles.waterfallGraph}>
          {waterfall.slice(-WATERFALL_HEIGHT).map((frame, y) => (
            <View key={y} style={{ flexDirection: 'row' }}>
              {frame.map((mag, x) => (
                <View
                  key={x}
                  style={{
                    width: binWidth,
                    height: 2,
                    backgroundColor: getWaterfallColor(mag, 1000),
                  }}
                />
              ))}
            </View>
          ))}
        </View>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      {/* Connection indicator */}
      <View style={styles.connectionBar}>
        <View
          style={[
            styles.connectionDot,
            { backgroundColor: connected ? '#00E676' : '#FF5252' },
          ]}
        />
        <Text style={styles.connectionText}>
          {connected ? 'Connected' : 'Disconnected'}
        </Text>
      </View>

      {!spectrum ? (
        <View style={styles.loadingContainer}>
          <ActivityIndicator size="large" color="#00E676" />
          <Text style={styles.loadingText}>Waiting for spectrum data...</Text>
        </View>
      ) : (
        <>
          {renderSpectrum()}
          {renderWaterfall()}
        </>
      )}

      {/* Controls */}
      <View style={styles.controls}>
        <TouchableOpacity style={styles.controlButton}>
          <Text style={styles.controlButtonText}>Peak Hold</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.controlButton}>
          <Text style={styles.controlButtonText}>Average</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.controlButton}>
          <Text style={styles.controlButtonText}>Max Hold</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

// Color helpers
function getColorForMagnitude(mag: number, max: number): string {
  const ratio = mag / max;
  if (ratio < 0.25) return '#1a237e';
  if (ratio < 0.5) return '#00bcd4';
  if (ratio < 0.75) return '#ffeb3b';
  return '#ff5722';
}

function getWaterfallColor(mag: number, max: number): string {
  const ratio = mag / max;
  if (ratio < 0.2) return '#0d0d2b';
  if (ratio < 0.4) return '#1a237e';
  if (ratio < 0.6) return '#00bcd4';
  if (ratio < 0.8) return '#ffeb3b';
  return '#ff5722';
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  connectionBar: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  connectionDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  connectionText: {
    color: '#888',
    fontSize: 14,
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    color: '#888',
    fontSize: 16,
    marginTop: 16,
  },
  sectionTitle: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  spectrumContainer: {
    marginBottom: 24,
  },
  spectrumGraph: {
    flexDirection: 'row',
    height: SPECTRUM_HEIGHT,
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    overflow: 'hidden',
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  freqLabels: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 4,
  },
  freqLabel: {
    color: '#666',
    fontSize: 10,
  },
  waterfallContainer: {
    marginBottom: 24,
  },
  waterfallGraph: {
    height: WATERFALL_HEIGHT,
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    overflow: 'hidden',
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  controls: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  controlButton: {
    backgroundColor: '#1a1a2e',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  controlButtonText: {
    color: '#fff',
    fontSize: 14,
  },
});

export default SpectrumScreen;
