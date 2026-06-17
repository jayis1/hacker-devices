/**
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * Analyze Screen — Review stored captures, decode, export
 *
 * Lists captures stored on the device and allows decoding/export.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
  FlatList,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import {
  buildRetrieveCapture,
  buildEraseStorage,
  MODE_NAMES,
} from '../utils/protocol';
import MessageLog from '../components/MessageLog';

const SERVICE_UUID = '00001810-0000-1000-8000-00805f9b34fb';
const CHARACTERISTIC_UUID = '00002a35-0000-1000-8000-00805f9b34fb';

/**
 * Analyze screen — review captured data from device storage.
 */
export default function AnalyzeScreen({ bleManager, connectedDevice }) {
  const [captures, setCaptures] = useState([]);
  const [selectedCapture, setSelectedCapture] = useState(null);
  const [decodedData, setDecodedData] = useState('');
  const [loading, setLoading] = useState(false);

  // Load capture list from device
  const refreshCaptures = useCallback(async () => {
    if (!connectedDevice) {
      Alert.alert('Not Connected', 'Connect to a device first.');
      return;
    }

    setLoading(true);

    // In production, we'd query all slots.
    // For now, simulate a few captures for development.
    const mockCaptures = [
      {
        id: 0,
        timestamp: Date.now() - 3600000,
        length: 128,
        modulation: 'FSK',
        quality: 85,
        data: 'DE AD BE EF CA FE BA BE 00 11 22 33 44 55 66 77',
      },
      {
        id: 1,
        timestamp: Date.now() - 7200000,
        length: 64,
        modulation: 'OOK',
        quality: 62,
        data: '01 02 03 04 05 06 07 08',
      },
    ];

    setCaptures(mockCaptures);
    setLoading(false);
  }, [connectedDevice]);

  // Decode a capture (placeholder — would use firmware-side processing)
  const decodeCapture = useCallback(
    async (capture) => {
      setSelectedCapture(capture);

      // In production, we'd send RETRIEVE_CAPTURE to the device
      if (connectedDevice) {
        try {
          await connectedDevice.writeCharacteristicWithResponseForService(
            SERVICE_UUID,
            CHARACTERISTIC_UUID,
            Buffer.from(buildRetrieveCapture(capture.id)).toString('base64'),
          );
        } catch (e) {
          // Use mock data for development
        }
      }

      // Simple hex decode for preview
      const hex = capture.data.replace(/\s+/g, '');
      const bytes = [];
      for (let i = 0; i < hex.length; i += 2) {
        bytes.push(parseInt(hex.substring(i, i + 2), 16));
      }

      // Try to decode as ASCII
      let ascii = '';
      for (const b of bytes) {
        if (b >= 32 && b <= 126) {
          ascii += String.fromCharCode(b);
        } else {
          ascii += '.';
        }
      }

      setDecodedData(
        `Raw (${bytes.length} bytes):\n${capture.data}\n\nASCII:\n${ascii}\n\nHex dump:`,
      );
    },
    [connectedDevice],
  );

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Capture List */}
      <View style={styles.card}>
        <View style={styles.listHeader}>
          <Text style={styles.cardTitle}>Stored Captures</Text>
          <TouchableOpacity onPress={refreshCaptures} disabled={loading}>
            <Icon name="refresh" size={20} color="#00d4ff" />
          </TouchableOpacity>
        </View>

        {captures.length === 0 && !loading && (
          <Text style={styles.emptyText}>
            No captures stored. Go to the Capture tab to record signals.
          </Text>
        )}

        {loading && <Text style={styles.loadingText}>Loading...</Text>}

        {captures.map((capture) => (
          <TouchableOpacity
            key={capture.id}
            style={[
              styles.captureItem,
              selectedCapture?.id === capture.id && styles.captureItemSelected,
            ]}
            onPress={() => decodeCapture(capture)}
          >
            <View style={styles.captureInfo}>
              <Text style={styles.captureMod}>{capture.modulation}</Text>
              <Text style={styles.captureTime}>
                {new Date(capture.timestamp).toLocaleTimeString()}
              </Text>
            </View>
            <View style={styles.captureMeta}>
              <Text style={styles.captureBytes}>
                {capture.length} bytes
              </Text>
              <View style={styles.qualityBadge}>
                <Text
                  style={[
                    styles.qualityText,
                    { color: capture.quality > 70 ? '#00c853' : '#ffaa00' },
                  ]}
                >
                  {capture.quality}%
                </Text>
              </View>
            </View>
            <Icon name="chevron-right" size={20} color="#6c757d" />
          </TouchableOpacity>
        ))}
      </View>

      {/* Selected Capture Detail */}
      {selectedCapture && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>
            Capture #{selectedCapture.id} — {selectedCapture.modulation}
          </Text>

          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Time:</Text>
            <Text style={styles.detailValue}>
              {new Date(selectedCapture.timestamp).toLocaleString()}
            </Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Length:</Text>
            <Text style={styles.detailValue}>
              {selectedCapture.length} bytes
            </Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Quality:</Text>
            <Text style={styles.detailValue}>{selectedCapture.quality}%</Text>
          </View>

          <View style={styles.decodedBox}>
            <Text style={styles.decodedLabel}>Decoded Data</Text>
            <Text style={styles.decodedText} selectable>
              {decodedData}
            </Text>
          </View>

          <View style={styles.actionRow}>
            <TouchableOpacity
              style={styles.actionBtn}
              onPress={() => {
                Alert.alert('Export', 'Capture data copied to clipboard (simulated)');
              }}
            >
              <Icon name="file-copy" size={18} color="#fff" />
              <Text style={styles.actionText}>Copy</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={[styles.actionBtn, { backgroundColor: '#441111' }]}
              onPress={() => {
                setSelectedCapture(null);
                setDecodedData('');
              }}
            >
              <Icon name="close" size={18} color="#ff4444" />
              <Text style={[styles.actionText, { color: '#ff4444' }]}>
                Clear
              </Text>
            </TouchableOpacity>
          </View>
        </View>
      )}

      {/* Message Log Recent */}
      <MessageLog messages={captures.slice(0, 5)} title="Recent Captures" />

      {/* Storage Management */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Storage Management</Text>
        <TouchableOpacity
          style={styles.eraseBtn}
          onPress={() => {
            Alert.alert(
              'Erase All Captures',
              'This will permanently delete all stored captures on the device. Continue?',
              [
                { text: 'Cancel', style: 'cancel' },
                {
                  text: 'Erase',
                  style: 'destructive',
                  onPress: async () => {
                    if (connectedDevice) {
                      try {
                        await connectedDevice.writeCharacteristicWithResponseForService(
                          SERVICE_UUID,
                          CHARACTERISTIC_UUID,
                          Buffer.from(buildEraseStorage()).toString('base64'),
                        );
                        setCaptures([]);
                        setSelectedCapture(null);
                        Alert.alert('Done', 'All captures erased.');
                      } catch (e) {
                        Alert.alert('Error', e.message);
                      }
                    }
                  },
                },
              ],
            );
          }}
        >
          <Icon name="delete-sweep" size={20} color="#ff4444" />
          <Text style={styles.eraseText}>Erase All Captures</Text>
        </TouchableOpacity>
      </View>
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
    marginBottom: 12,
  },
  listHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  emptyText: {
    color: '#6c757d',
    fontSize: 13,
    textAlign: 'center',
    paddingVertical: 20,
  },
  loadingText: {
    color: '#00d4ff',
    fontSize: 13,
    textAlign: 'center',
    paddingVertical: 10,
  },
  captureItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 6,
    gap: 10,
  },
  captureItemSelected: {
    borderWidth: 1,
    borderColor: '#00d4ff',
  },
  captureInfo: {
    flex: 1,
  },
  captureMod: {
    color: '#e0e0e0',
    fontSize: 14,
    fontWeight: '600',
  },
  captureTime: {
    color: '#6c757d',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  captureMeta: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  captureBytes: {
    color: '#b0b0b0',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  qualityBadge: {
    backgroundColor: '#1a2a1a',
    borderRadius: 4,
    paddingHorizontal: 6,
    paddingVertical: 2,
  },
  qualityText: {
    fontSize: 11,
    fontWeight: 'bold',
  },
  detailRow: {
    flexDirection: 'row',
    marginBottom: 4,
  },
  detailLabel: {
    color: '#6c757d',
    fontSize: 12,
    width: 60,
  },
  detailValue: {
    color: '#b0b0b0',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  decodedBox: {
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    padding: 12,
    marginTop: 10,
    borderWidth: 1,
    borderColor: '#16213e',
  },
  decodedLabel: {
    color: '#00d4ff',
    fontSize: 11,
    fontWeight: '600',
    marginBottom: 6,
  },
  decodedText: {
    color: '#b0b0b0',
    fontSize: 11,
    fontFamily: 'monospace',
    lineHeight: 16,
  },
  actionRow: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 12,
  },
  actionBtn: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 10,
    gap: 6,
    borderWidth: 1,
    borderColor: '#333',
  },
  actionText: {
    color: '#fff',
    fontSize: 13,
    fontWeight: '600',
  },
  eraseBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#2a1a1a',
    borderRadius: 8,
    padding: 12,
    gap: 8,
    borderWidth: 1,
    borderColor: '#441111',
  },
  eraseText: {
    color: '#ff4444',
    fontSize: 14,
    fontWeight: '600',
  },
});