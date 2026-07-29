/**
 * screens/ConnectionScreen.js — BLE scan & connect
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ActivityIndicator, Alert } from 'react-native';

export default function ConnectionScreen({ ble, componentId }) {
  const [scanning, setScanning] = useState(false);
  const [status, setStatus] = useState('Disconnected');

  const handleScan = async () => {
    setScanning(true);
    setStatus('Scanning for NVMe-Phantom...');
    try {
      await ble.scanAndConnect();
      setStatus('Connected');
      setScanning(false);
      Alert.alert('Connected', 'NVMe-Phantom device connected', [
        { text: 'OK', onPress: () => Navigation.push(componentId, { component: { name: 'Dashboard' } }) }
      ]);
    } catch (e) {
      setStatus('Error: ' + e.message);
      setScanning(false);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>NVMe-Phantom</Text>
      <Text style={styles.subtitle}>M.2 NVMe Storage Interposer</Text>
      <Text style={styles.author}>by jayis1</Text>
      <Text style={styles.status}>{status}</Text>
      {scanning ? (
        <ActivityIndicator size="large" color="#00AA00" />
      ) : (
        <TouchableOpacity style={styles.button} onPress={handleScan}>
          <Text style={styles.buttonText}>Scan & Connect</Text>
        </TouchableOpacity>
      )}
      <Text style={styles.legal}>
        ⚠️ Authorized security research only.{'\n'}
        Use only on systems you own or are{'\n'}
        explicitly authorized to assess.
      </Text>
    </View>
  );
}

const Navigation = require('react-native-navigation').Navigation;

const styles = StyleSheet.create({
  container: { flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#1a1a2e', padding: 20 },
  title: { fontSize: 28, fontWeight: 'bold', color: '#00AA00', marginBottom: 5 },
  subtitle: { fontSize: 16, color: '#888', marginBottom: 2 },
  author: { fontSize: 12, color: '#666', marginBottom: 40 },
  status: { fontSize: 14, color: '#aaa', marginBottom: 20, textAlign: 'center' },
  button: { backgroundColor: '#00AA00', padding: 15, borderRadius: 8, width: 200, alignItems: 'center' },
  buttonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  legal: { fontSize: 11, color: '#FF6600', marginTop: 40, textAlign: 'center' },
});