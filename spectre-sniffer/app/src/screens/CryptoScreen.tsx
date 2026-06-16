//==============================================================================
// CryptoScreen.tsx — Crypto Analysis View
// Author: jayis1
// Description: Interface for capturing EM traces during cryptographic
//              operations and running DPA/CPA analysis to recover keys.
//==============================================================================

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
} from 'react-native';
import { SpectreAPI } from '../services/api';
import { CryptoAnalysisResult } from '../types';

const CryptoScreen: React.FC = () => {
  const [numTraces, setNumTraces] = useState(1000);
  const [algorithm, setAlgorithm] = useState('AES-128');
  const [analysisType, setAnalysisType] = useState<'DPA' | 'CPA'>('DPA');
  const [result, setResult] = useState<CryptoAnalysisResult | null>(null);
  const [loading, setLoading] = useState(false);
  const [capturing, setCapturing] = useState(false);

  const startCapture = useCallback(async () => {
    setCapturing(true);
    try {
      // Trigger device to capture crypto traces
      // (In production, this would coordinate with the target device)
      await new Promise((resolve) => setTimeout(resolve, 2000));
      setCapturing(false);
    } catch (err) {
      setCapturing(false);
    }
  }, []);

  const runAnalysis = useCallback(async () => {
    setLoading(true);
    try {
      const data = await SpectreAPI.runCryptoAnalysis({
        algorithm,
        numTraces,
        analysisType,
      });
      setResult(data);
    } catch (err) {
      console.error('Analysis failed:', err);
    }
    setLoading(false);
  }, [algorithm, numTraces, analysisType]);

  const formatKey = (bytes: number[]): string => {
    if (!bytes || bytes.length === 0) return 'No key recovered';
    return bytes
      .map((b) => b.toString(16).padStart(2, '0'))
      .join(' ')
      .toUpperCase();
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Cryptographic Side-Channel Analysis</Text>
      <Text style={styles.description}>
        Capture EM traces during cryptographic operations and recover secret keys
        using Differential/Correlation Power Analysis.
      </Text>

      {/* Configuration */}
      <View style={styles.configCard}>
        <Text style={styles.configTitle}>Analysis Configuration</Text>

        <View style={styles.configRow}>
          <Text style={styles.configLabel}>Algorithm:</Text>
          <TouchableOpacity
            style={styles.configValue}
            onPress={() => {
              const algos = ['AES-128', 'AES-256', 'DES', '3DES'];
              const idx = algos.indexOf(algorithm);
              setAlgorithm(algos[(idx + 1) % algos.length]);
            }}
          >
            <Text style={styles.configValueText}>{algorithm}</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.configRow}>
          <Text style={styles.configLabel}>Analysis Type:</Text>
          <TouchableOpacity
            style={styles.configValue}
            onPress={() =>
              setAnalysisType((prev) => (prev === 'DPA' ? 'CPA' : 'DPA'))
            }
          >
            <Text style={styles.configValueText}>{analysisType}</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.configRow}>
          <Text style={styles.configLabel}>Number of Traces:</Text>
          <TouchableOpacity
            style={styles.configValue}
            onPress={() => {
              const options = [100, 500, 1000, 5000, 10000, 50000];
              const idx = options.indexOf(numTraces);
              setNumTraces(options[(idx + 1) % options.length]);
            }}
          >
            <Text style={styles.configValueText}>
              {numTraces.toLocaleString()}
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Action Buttons */}
      <View style={styles.buttonRow}>
        <TouchableOpacity
          style={[styles.button, styles.captureButton]}
          onPress={startCapture}
          disabled={capturing}
        >
          {capturing ? (
            <ActivityIndicator color="#fff" />
          ) : (
            <Text style={styles.buttonText}>Capture Traces</Text>
          )}
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.button, styles.analyzeButton]}
          onPress={runAnalysis}
          disabled={loading || capturing}
        >
          {loading ? (
            <ActivityIndicator color="#fff" />
          ) : (
            <Text style={styles.buttonText}>Run {analysisType}</Text>
          )}
        </TouchableOpacity>
      </View>

      {/* Results */}
      {result && (
        <View style={styles.resultCard}>
          <Text style={styles.resultTitle}>Analysis Results</Text>

          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Status:</Text>
            <Text
              style={[
                styles.resultValue,
                { color: result.running ? '#FFC107' : '#00E676' },
              ]}
            >
              {result.running ? 'Running...' : 'Complete'}
            </Text>
          </View>

          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Traces Used:</Text>
            <Text style={styles.resultValue}>
              {result.numTraces.toLocaleString()}
            </Text>
          </View>

          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Progress:</Text>
            <Text style={styles.resultValue}>
              {(result.progress * 100).toFixed(0)}%
            </Text>
          </View>

          <Text style={styles.keyTitle}>Recovered Key:</Text>
          <View style={styles.keyDisplay}>
            <Text style={styles.keyText}>{formatKey(result.keyBytes)}</Text>
          </View>

          {/* Confidence bars */}
          <Text style={styles.confidenceTitle}>Per-Byte Confidence:</Text>
          {result.confidence.map((conf, i) => (
            <View key={i} style={styles.confidenceRow}>
              <Text style={styles.confidenceLabel}>Byte {i}:</Text>
              <View style={styles.confidenceBar}>
                <View
                  style={[
                    styles.confidenceFill,
                    {
                      width: `${Math.min(conf * 100, 100)}%`,
                      backgroundColor:
                        conf > 0.7
                          ? '#00E676'
                          : conf > 0.4
                          ? '#FFC107'
                          : '#FF5252',
                    },
                  ]}
                />
              </View>
              <Text style={styles.confidenceValue}>
                {(conf * 100).toFixed(0)}%
              </Text>
            </View>
          ))}
        </View>
      )}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
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
  configCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  configTitle: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  configRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a4e',
  },
  configLabel: {
    color: '#aaa',
    fontSize: 14,
  },
  configValue: {
    backgroundColor: '#0f0f23',
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderRadius: 6,
  },
  configValueText: {
    color: '#00E676',
    fontSize: 14,
    fontWeight: '600',
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 24,
  },
  button: {
    paddingHorizontal: 24,
    paddingVertical: 12,
    borderRadius: 8,
    minWidth: 140,
    alignItems: 'center',
  },
  captureButton: {
    backgroundColor: '#448AFF',
  },
  analyzeButton: {
    backgroundColor: '#E040FB',
  },
  buttonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  resultCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 24,
    borderWidth: 1,
    borderColor: '#2a2a4e',
  },
  resultTitle: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  resultRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  resultLabel: {
    color: '#888',
    fontSize: 14,
  },
  resultValue: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  keyTitle: {
    color: '#FFC107',
    fontSize: 14,
    fontWeight: 'bold',
    marginTop: 12,
    marginBottom: 8,
  },
  keyDisplay: {
    backgroundColor: '#0f0f23',
    padding: 12,
    borderRadius: 8,
    marginBottom: 12,
  },
  keyText: {
    color: '#00E676',
    fontSize: 13,
    fontFamily: 'monospace',
    letterSpacing: 2,
  },
  confidenceTitle: {
    color: '#aaa',
    fontSize: 13,
    fontWeight: '600',
    marginBottom: 8,
  },
  confidenceRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 4,
  },
  confidenceLabel: {
    color: '#888',
    fontSize: 11,
    width: 50,
  },
  confidenceBar: {
    flex: 1,
    height: 8,
    backgroundColor: '#0f0f23',
    borderRadius: 4,
    overflow: 'hidden',
    marginHorizontal: 8,
  },
  confidenceFill: {
    height: '100%',
    borderRadius: 4,
  },
  confidenceValue: {
    color: '#888',
    fontSize: 11,
    width: 40,
    textAlign: 'right',
  },
});

export default CryptoScreen;
