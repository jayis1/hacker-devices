/**
 * CovertChannelScreen.js — CC-line VDM covert channel monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Monitors and controls the CC-line VDM covert channel. Data is
 * exfiltrated from a compromised target device via PD Vendor Defined
 * Messages on the USB-C Configuration Channel — invisible to USB
 * monitoring, EDR, and network DLP.
 *
 * This screen can:
 *  - Start/stop monitoring for incoming VDM covert data
 *  - Send data over the covert channel (for bidirectional C2)
 *  - View received data in hex and ASCII
 *  - Display channel statistics (bytes TX/RX, throughput)
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, TextInput, FlatList } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function CovertChannelScreen() {
  const { sendCommand, setStream } = useDevice();
  const [monitoring, setMonitoring] = useState(false);
  const [txInput, setTxInput] = useState('');
  const [rxData, setRxData] = useState([]);
  const [txCount, setTxCount] = useState(0);
  const [rxCount, setRxCount] = useState(0);
  const [status, setStatus] = useState('Idle');

  // Handle incoming covert channel data
  useEffect(() => {
    if (monitoring) {
      setStream((line) => {
        if (line.startsWith('COVERT_RX')) {
          const match = line.match(/COVERT_RX (\d+) bytes/);
          if (match) {
            const bytes = parseInt(match[1]);
            setRxCount((prev) => prev + bytes);
            setRxData((prev) => [...prev.slice(-50), {
              time: new Date().toLocaleTimeString(),
              bytes: bytes,
              data: '(binary data received)',
            }]);
          }
        }
      });
    } else {
      setStream(null);
    }
  }, [monitoring, setStream]);

  const startMonitoring = async () => {
    const resp = await sendCommand('COVERT_RX_START', 2000);
    if (resp && resp.startsWith('OK')) {
      setMonitoring(true);
      setStatus('Monitoring for VDM data...');
    }
  };

  const stopMonitoring = async () => {
    const resp = await sendCommand('COVERT_RX_STOP', 2000);
    if (resp && resp.startsWith('OK')) {
      setMonitoring(false);
      setStatus('Monitoring stopped');
    }
  };

  const sendTx = async () => {
    if (!txInput.trim()) return;
    // Convert text to hex string
    let hex = '';
    for (let i = 0; i < txInput.length; i++) {
      hex += txInput.charCodeAt(i).toString(16).padStart(2, '0');
    }
    const resp = await sendCommand(`COVERT_TX ${hex}`, 3000);
    if (resp && resp.startsWith('OK')) {
      setTxCount((prev) => prev + txInput.length);
      setRxData((prev) => [...prev.slice(-50), {
        time: new Date().toLocaleTimeString(),
        bytes: txInput.length,
        data: `TX: "${txInput}"`,
      }]);
      setTxInput('');
    }
  };

  const clearLog = () => {
    setRxData([]);
    setTxCount(0);
    setRxCount(0);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>CC Covert Channel</Text>
      <Text style={styles.subtitle}>VDM-based data exfiltration over USB-C CC line</Text>

      <View style={styles.statusCard}>
        <Text style={styles.statusLabel}>Status: {status}</Text>
        <Text style={styles.statsText}>TX: {txCount} bytes | RX: {rxCount} bytes</Text>
      </View>

      <View style={styles.buttonRow}>
        <TouchableOpacity
          style={[styles.button, monitoring ? styles.buttonActive : styles.buttonStart]}
          onPress={monitoring ? stopMonitoring : startMonitoring}
        >
          <Text style={styles.buttonText}>
            {monitoring ? 'Stop Monitoring' : 'Start Monitoring'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.buttonClear} onPress={clearLog}>
          <Text style={styles.buttonText}>Clear</Text>
        </TouchableOpacity>
      </View>

      {/* TX section */}
      <Text style={styles.sectionTitle}>Send Data (TX)</Text>
      <View style={styles.inputRow}>
        <TextInput
          style={styles.input}
          value={txInput}
          onChangeText={setTxInput}
          placeholder="Enter data to send..."
          placeholderTextColor="#6e7681"
        />
        <TouchableOpacity style={styles.sendButton} onPress={sendTx}>
          <Text style={styles.buttonText}>Send</Text>
        </TouchableOpacity>
      </View>

      {/* RX log */}
      <Text style={styles.sectionTitle}>Data Log</Text>
      <View style={styles.logContainer}>
        {rxData.length === 0 ? (
          <Text style={styles.emptyText}>No data received. Start monitoring to capture VDM covert channel data.</Text>
        ) : (
          rxData.map((entry, i) => (
            <View key={i} style={styles.logEntry}>
              <Text style={styles.logTime}>{entry.time}</Text>
              <Text style={styles.logData}>{entry.data}</Text>
            </View>
          ))
        )}
      </View>

      <Text style={styles.disclaimer}>
        ⚠️ The covert channel is for testing exfiltration detection on systems you are authorized to assess.
        Using this to exfiltrate data from unauthorized systems may violate wiretapping and data-protection laws.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#00d4aa' },
  subtitle: { fontSize: 12, color: '#8b949e', marginBottom: 16 },
  statusCard: { backgroundColor: '#161b22', padding: 12, borderRadius: 8, marginBottom: 12, borderWidth: 1, borderColor: '#30363d' },
  statusLabel: { color: '#e6edf3', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  statsText: { color: '#58a6ff', fontSize: 12 },
  buttonRow: { flexDirection: 'row', gap: 8, marginBottom: 16 },
  button: { flex: 1, padding: 12, borderRadius: 8, alignItems: 'center' },
  buttonStart: { backgroundColor: '#238636' },
  buttonActive: { backgroundColor: '#f85149' },
  buttonClear: { backgroundColor: '#21262d', padding: 12, borderRadius: 8, alignItems: 'center', width: 80 },
  buttonText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  sectionTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  inputRow: { flexDirection: 'row', gap: 8, alignItems: 'center' },
  input: { flex: 1, backgroundColor: '#161b22', color: '#e6edf3', padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#30363d', fontSize: 13 },
  sendButton: { backgroundColor: '#1f6feb', padding: 10, borderRadius: 6, paddingHorizontal: 16 },
  logContainer: { backgroundColor: '#161b22', borderRadius: 8, padding: 8, minHeight: 200, borderWidth: 1, borderColor: '#30363d' },
  logEntry: { flexDirection: 'row', gap: 8, marginBottom: 4 },
  logTime: { color: '#6e7681', fontSize: 11, fontFamily: 'monospace' },
  logData: { color: '#e6edf3', fontSize: 11, fontFamily: 'monospace' },
  emptyText: { color: '#6e7681', fontSize: 12, textAlign: 'center', marginTop: 40 },
  disclaimer: { color: '#f85149', fontSize: 10, marginTop: 16, textAlign: 'center' },
});