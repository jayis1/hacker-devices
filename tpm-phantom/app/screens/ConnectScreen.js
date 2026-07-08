/*
 * tpm-phantom — app/screens/ConnectScreen.js
 * BLE device discovery and connection screen.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet, ActivityIndicator } from 'react-native';
import ble from '../utils/bleManager';

export default function ConnectScreen({ onConnected }) {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connecting, setConnecting] = useState(false);
  const [statusMsg, setStatusMsg] = useState('Tap Scan to search for tpm-phantom devices');

  useEffect(() => {
    ble.onStatus = (type, msg) => {
      if (type === 'connected') {
        setStatusMsg(`Connected to ${msg}`);
        onConnected();
      } else if (type === 'disconnected') {
        setStatusMsg('Disconnected');
      } else if (type === 'error') {
        setStatusMsg(`Error: ${msg}`);
        setConnecting(false);
      }
    };
  }, [onConnected]);

  const handleScan = useCallback(() => {
    setScanning(true);
    setDevices([]);
    setStatusMsg('Scanning for tpm-phantom devices...');

    ble.scan((device) => {
      setDevices(prev => {
        if (prev.find(d => d.id === device.id)) return prev;
        return [...prev, device];
      });
      setStatusMsg(`Found: ${device.name || 'Unknown'}`);
    }, 10000);

    setTimeout(() => {
      setScanning(false);
      if (devices.length === 0) {
        setStatusMsg('No devices found. Ensure tpm-phantom is powered on.');
      }
    }, 10000);
  }, []);

  const handleConnect = useCallback(async (device) => {
    setConnecting(true);
    setStatusMsg(`Connecting to ${device.name || device.id}...`);
    const ok = await ble.connect(device);
    if (ok) {
      onConnected();
    }
    setConnecting(false);
  }, [onConnected]);

  const renderDevice = ({ item }) => (
    <TouchableOpacity
      style={styles.deviceRow}
      onPress={() => handleConnect(item)}
      disabled={connecting}>
      <View>
        <Text style={styles.deviceName}>{item.name || 'Unknown Device'}</Text>
        <Text style={styles.deviceId}>{item.id}</Text>
        <Text style={styles.deviceRssi}>RSSI: {item.rssi} dBm</Text>
      </View>
      <Text style={styles.connectArrow}>→</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>tpm-phantom</Text>
        <Text style={styles.subtitle}>Trusted Platform Module Bus Analyzer</Text>
        <Text style={styles.author}>by jayis1</Text>
      </View>

      <TouchableOpacity
        style={[styles.scanBtn, scanning ? styles.scanBtnActive : null]}
        onPress={handleScan}
        disabled={scanning || connecting}>
        {scanning ? (
          <ActivityIndicator color="#fff" />
        ) : (
          <Text style={styles.scanBtnText}>Scan for Devices</Text>
        )}
      </TouchableOpacity>

      <Text style={styles.status}>{statusMsg}</Text>

      {devices.length > 0 ? (
        <FlatList
          data={devices}
          renderItem={renderDevice}
          keyExtractor={item => item.id}
          style={styles.deviceList}
        />
      ) : (
        !scanning && (
          <View style={styles.helpCard}>
            <Text style={styles.helpTitle}>Getting Started</Text>
            <Text style={styles.helpText}>
              1. Power on the tpm-phantom device (USB or battery){'\n'}
              2. Connect the LPC or SPI probe to the target TPM{'\n'}
              3. Tap "Scan" to find the device via BLE{'\n'}
              4. Tap a device to connect{'\n'}
              5. Use the Dashboard to start capturing{'\n\n'}
              The device advertises as "tpm-phantom-XXXX" over BLE.
            </Text>
          </View>
        )
      )}

      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠ For authorized security research only. Unauthorized TPM
          bus interception may violate computer fraud laws. Author: jayis1
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { alignItems: 'center', paddingVertical: 30 },
  title: {
    color: '#00ff88',
    fontSize: 28,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  subtitle: {
    color: '#8888ff',
    fontSize: 14,
    fontFamily: 'monospace',
    marginTop: 4,
  },
  author: {
    color: '#666',
    fontSize: 12,
    fontFamily: 'monospace',
    marginTop: 2,
  },
  scanBtn: {
    backgroundColor: '#0a5',
    padding: 16,
    borderRadius: 10,
    alignItems: 'center',
    marginHorizontal: 20,
    marginBottom: 12,
  },
  scanBtnActive: { backgroundColor: '#053' },
  scanBtnText: { color: '#fff', fontSize: 16, fontFamily: 'monospace', fontWeight: 'bold' },
  status: {
    color: '#aaa',
    fontSize: 12,
    fontFamily: 'monospace',
    textAlign: 'center',
    marginBottom: 10,
  },
  deviceList: { flex: 1, marginHorizontal: 12 },
  deviceRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    padding: 14,
    borderRadius: 8,
    marginBottom: 6,
    borderWidth: 1,
    borderColor: '#333',
  },
  deviceName: { color: '#00ff88', fontSize: 15, fontFamily: 'monospace', fontWeight: 'bold' },
  deviceId: { color: '#888', fontSize: 11, fontFamily: 'monospace', marginTop: 2 },
  deviceRssi: { color: '#ffdd44', fontSize: 10, fontFamily: 'monospace', marginTop: 2 },
  connectArrow: { color: '#0a5', fontSize: 24 },
  helpCard: {
    backgroundColor: '#1a1a2e',
    margin: 16,
    padding: 16,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  helpTitle: {
    color: '#8888ff',
    fontSize: 16,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    marginBottom: 8,
  },
  helpText: {
    color: '#aaa',
    fontSize: 12,
    fontFamily: 'monospace',
    lineHeight: 18,
  },
  disclaimer: {
    margin: 12,
    padding: 10,
    backgroundColor: '#221111',
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#ff3344',
  },
  disclaimerText: {
    color: '#ff6666',
    fontSize: 10,
    fontFamily: 'monospace',
    textAlign: 'center',
  },
});