/**
 * HomeScreen.tsx — PhotonStrike home / connection screen
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { useBle } from '../services/BleContext';

export default function HomeScreen({ navigation }: any) {
  const ble = useBle();

  const onConnect = async () => {
    try {
      await ble.connect();
    } catch (e: any) {
      Alert.alert('Connection failed', String(e?.message ?? e));
    }
  };

  const onEStop = async () => {
    await ble.emergencyStop();
    Alert.alert('EMERGENCY STOP', 'Laser disabled, scan aborted.');
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>PhotonStrike</Text>
      <Text style={styles.subtitle}>Portable Laser Fault Injection</Text>
      <Text style={styles.author}>by jayis1</Text>

      <View style={styles.statusRow}>
        <Text style={styles.label}>BLE:</Text>
        <Text style={[styles.value, { color: ble.connected ? '#4ecca3' : '#e94560' }]}>
          {ble.connected ? 'Connected' : ble.connecting ? 'Connecting…' : 'Disconnected'}
        </Text>
      </View>

      {!ble.connected ? (
        <TouchableOpacity style={styles.button} onPress={onConnect}>
          <Text style={styles.buttonText}>Connect to PhotonStrike</Text>
        </TouchableOpacity>
      ) : (
        <View style={styles.buttonGroup}>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('ScanSetup')}>
            <Text style={styles.buttonText}>Scan Setup</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('Trigger')}>
            <Text style={styles.buttonText}>Trigger Config</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('ScanProgress')}>
            <Text style={styles.buttonText}>Scan Progress</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('DFA')}>
            <Text style={styles.buttonText}>DFA Recovery</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('Log')}>
            <Text style={styles.buttonText}>Scan Logs</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('Safety')}>
            <Text style={styles.buttonText}>Laser Safety</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.button, { backgroundColor: '#e94560' }]} onPress={onEStop}>
            <Text style={styles.buttonText}>EMERGENCY STOP</Text>
          </TouchableOpacity>
        </View>
      )}

      <Text style={styles.disclaimer}>
        ⚠️ Class 3B/4 laser. Authorized research use only. Wear 1064 nm goggles.
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, justifyContent: 'center' },
  title: { fontSize: 32, fontWeight: 'bold', color: '#e94560', textAlign: 'center' },
  subtitle: { fontSize: 16, color: '#a0a0c0', textAlign: 'center', marginTop: 4 },
  author: { fontSize: 12, color: '#666', textAlign: 'center', marginTop: 2 },
  statusRow: { flexDirection: 'row', justifyContent: 'center', marginVertical: 20 },
  label: { color: '#a0a0c0', fontSize: 16, marginRight: 8 },
  value: { fontSize: 16, fontWeight: 'bold' },
  button: {
    backgroundColor: '#16213e', padding: 14, borderRadius: 8,
    alignItems: 'center', marginVertical: 6, borderWidth: 1, borderColor: '#e94560',
  },
  buttonText: { color: '#e94560', fontSize: 16, fontWeight: '600' },
  buttonGroup: { marginTop: 10 },
  disclaimer: {
    color: '#e94560', fontSize: 11, textAlign: 'center',
    marginTop: 30, paddingHorizontal: 20,
  },
});