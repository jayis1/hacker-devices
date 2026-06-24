/*
 * lora-phantom / app / screens / ConnectionScreen.js
 * BLE scan/pair + USB attach screen.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function ConnectionScreen({ navigation }) {
  const { connected, transport, connect, disconnect, status } = useDevice();
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);

  const SERVICE_UUID = 'deadf00d-dead-f00d-dead-f00ddeadf00d';

  const startBleScan = async () => {
    setScanning(true);
    setDevices([]);
    // In a full implementation, use react-native-ble-plx:
    //   manager.startDeviceScan([SERVICE_UUID], null, (err, dev) => { ... })
    // For the prototype, we simulate finding the device.
    setTimeout(() => {
      setDevices([
        { id: 'AA:BB:CC:DD:EE:FF', name: 'LoRaPhantom-001', rssi: -45 },
      ]);
      setScanning(false);
    }, 1500);
  };

  const connectBle = async (devId) => {
    try {
      await connect('ble');
      navigation.navigate('Dashboard');
    } catch (e) {
      Alert.alert('BLE Error', e.message);
    }
  };

  const connectUsb = async () => {
    try {
      await connect('usb');
      navigation.navigate('Dashboard');
    } catch (e) {
      Alert.alert('USB Error', e.message);
    }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>LoRaPhantom</Text>
      <Text style={styles.subtitle}>LoRaWAN & LoRa Infiltration Device</Text>
      <Text style={styles.author}>by jayis1</Text>

      {connected ? (
        <View style={styles.connectedBox}>
          <Text style={styles.connectedText}>
            ✅ Connected via {transport?.toUpperCase()}
          </Text>
          {status && (
            <Text style={styles.statusText}>
              Mode: {status.mode} | Battery: {status.batMv}mV
            </Text>
          )}
          <TouchableOpacity style={styles.button} onPress={disconnect}>
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('Dashboard')}>
            <Text style={styles.buttonText}>Go to Dashboard →</Text>
          </TouchableOpacity>
        </View>
      ) : (
        <View>
          <Text style={styles.section}>Connect via BLE</Text>
          <TouchableOpacity style={styles.button} onPress={startBleScan} disabled={scanning}>
            <Text style={styles.buttonText}>{scanning ? 'Scanning...' : 'Scan for BLE Devices'}</Text>
          </TouchableOpacity>

          {devices.map(d => (
            <TouchableOpacity key={d.id} style={styles.deviceItem} onPress={() => connectBle(d.id)}>
              <Text style={styles.deviceName}>{d.name || 'Unknown'}</Text>
              <Text style={styles.deviceId}>{d.id}</Text>
              <Text style={styles.deviceRssi}>RSSI: {d.rssi} dBm</Text>
            </TouchableOpacity>
          ))}

          <Text style={styles.section}>Connect via USB-C</Text>
          <TouchableOpacity style={styles.button} onPress={connectUsb}>
            <Text style={styles.buttonText}>Attach USB CDC</Text>
          </TouchableOpacity>

          <View style={styles.disclaimer}>
            <Text style={styles.disclaimerText}>
              ⚠️ LEGAL DISCLAIMER: This device is for authorized security
              research and penetration testing ONLY. Unauthorized interception
              or transmission on LoRa/LoRaWAN networks may violate laws
              (CFAA, wiretap statutes, radio regulations). Always obtain
              written authorization before deployment.
            </Text>
          </View>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 20 },
  title: { fontSize: 28, fontWeight: 'bold', color: '#00ff88', textAlign: 'center', marginTop: 20 },
  subtitle: { fontSize: 14, color: '#888', textAlign: 'center', marginTop: 5 },
  author: { fontSize: 12, color: '#555', textAlign: 'center', marginTop: 3 },
  section: { fontSize: 18, fontWeight: 'bold', color: '#ccc', marginTop: 25, marginBottom: 10 },
  button: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
            borderRadius: 8, padding: 15, alignItems: 'center', marginBottom: 10 },
  buttonText: { color: '#00ff88', fontSize: 16, fontWeight: '600' },
  connectedBox: { backgroundColor: '#1a1a1a', borderRadius: 8, padding: 20, marginTop: 20 },
  connectedText: { color: '#00ff88', fontSize: 18, fontWeight: 'bold', marginBottom: 10 },
  statusText: { color: '#aaa', fontSize: 14, marginBottom: 15 },
  deviceItem: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#333',
                borderRadius: 6, padding: 15, marginBottom: 8 },
  deviceName: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  deviceId: { color: '#888', fontSize: 12, marginTop: 3 },
  deviceRssi: { color: '#00ff88', fontSize: 12, marginTop: 3 },
  disclaimer: { backgroundColor: '#2a1a0a', borderWidth: 1, borderColor: '#ff6600',
                borderRadius: 8, padding: 15, marginTop: 30 },
  disclaimerText: { color: '#ffaa44', fontSize: 11 },
});