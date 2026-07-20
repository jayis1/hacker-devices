/**
 * hart-bleeder — DashboardScreen.js
 * Main dashboard: connection status, device state, battery, loop
 * current, device count, and quick connect/disarm controls.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Alert,
  SafeAreaView, StatusBar, ActivityIndicator,
} from 'react-native';

export default function DashboardScreen({ navigation, bleManager, connected,
  deviceState, battery, nDevices, loopCurrent, armed, setArmed }) {
  const [scanning, setScanning] = useState(false);

  const handleConnect = async () => {
    setScanning(true);
    try {
      await bleManager.scanAndConnect();
    } catch (e) {
      Alert.alert('Connection Error', e.message);
    }
    setScanning(false);
  };

  const handleArm = async () => {
    if (!armed) {
      Alert.alert(
        'Arm Device',
        'Arming enables destructive operations (setpoint writes, DoS, spoofing). Continue?',
        [
          { text: 'Cancel', style: 'cancel' },
          {
            text: 'ARM',
            style: 'destructive',
            onPress: async () => {
              try {
                await bleManager.setMode(1);
                setArmed(true);
              } catch (e) { Alert.alert('Error', e.message); }
            },
          },
        ]
      );
    } else {
      try {
        await bleManager.goPassive();
        setArmed(false);
      } catch (e) { Alert.alert('Error', e.message); }
    }
  };

  const stateColor = {
    'IDLE': '#4a90d9',
    'ARMED': '#e94560',
    'PASSIVE_MONITOR': '#10b981',
    'ACTIVE': '#e94560',
    'FAULT': '#f59e0b',
  };
  const color = stateColor[deviceState] || '#4a90d9';

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <View style={styles.header}>
        <Text style={styles.title}>HART-Bleeder</Text>
        <Text style={styles.subtitle}>HART Fieldbus Attack Implant</Text>
        <Text style={styles.author}>by jayis1</Text>
      </View>

      <View style={[styles.statusCard, { borderColor: color }]}>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Connection</Text>
          <Text style={[styles.value, { color: connected ? '#10b981' : '#e94560' }]}>
            {connected ? '● CONNECTED' : '○ DISCONNECTED'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>State</Text>
          <Text style={[styles.value, { color }]}>{deviceState}</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Battery</Text>
          <Text style={styles.value}>{battery}%</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Loop Current</Text>
          <Text style={styles.value}>{loopCurrent.toFixed(2)} mA</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Devices Found</Text>
          <Text style={styles.value}>{nDevices}</Text>
        </View>
        <View style={styles.statusRow}>
          <Text style={styles.label}>Armed</Text>
          <Text style={[styles.value, { color: armed ? '#e94560' : '#10b981' }]}>
            {armed ? 'YES' : 'NO'}
          </Text>
        </View>
      </View>

      {!connected ? (
        <TouchableOpacity style={styles.buttonPrimary} onPress={handleConnect}>
          {scanning ? (
            <ActivityIndicator color="#fff" />
          ) : (
            <Text style={styles.buttonText}>Connect to Device</Text>
          )}
        </TouchableOpacity>
      ) : (
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.button, armed ? styles.buttonDanger : styles.buttonWarn]}
            onPress={handleArm}
          >
            <Text style={styles.buttonText}>
              {armed ? 'DISARM' : 'ARM'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.buttonSecondary]}
            onPress={() => bleManager.disconnect()}
          >
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
        </View>
      )}

      <View style={styles.navGrid}>
        <NavButton label="Bus Scan"    onPress={() => navigation.navigate('DeviceScan')} />
        <NavButton label="Live Capture" onPress={() => navigation.navigate('LiveCapture')} />
        <NavButton label="Loop Control" onPress={() => navigation.navigate('LoopControl')} />
        <NavButton label="Attacks"      onPress={() => navigation.navigate('AttackConsole')} />
        <NavButton label="Session Log"  onPress={() => navigation.navigate('SessionLog')} />
        <NavButton label="Settings"     onPress={() => navigation.navigate('Settings')} />
      </View>

      <Text style={styles.disclaimer}>
        ⚠ Authorized security research only. Use on equipment you own or
        have explicit written permission to test.
      </Text>
    </SafeAreaView>
  );
}

function NavButton({ label, onPress }) {
  return (
    <TouchableOpacity style={styles.navButton} onPress={onPress}>
      <Text style={styles.navButtonText}>{label}</Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  header: { alignItems: 'center', marginBottom: 16 },
  title: { fontSize: 26, fontWeight: 'bold', color: '#e94560' },
  subtitle: { fontSize: 12, color: '#888', marginTop: 2 },
  author: { fontSize: 10, color: '#555' },
  statusCard: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16,
    borderWidth: 2, marginBottom: 16,
  },
  statusRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  label: { color: '#aaa', fontSize: 14 },
  value: { color: '#fff', fontSize: 14, fontWeight: '600' },
  buttonPrimary: {
    backgroundColor: '#e94560', padding: 16, borderRadius: 10,
    alignItems: 'center', marginBottom: 12,
  },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  button: { flex: 1, padding: 14, borderRadius: 10, alignItems: 'center', marginHorizontal: 4 },
  buttonWarn: { backgroundColor: '#f59e0b' },
  buttonDanger: { backgroundColor: '#e94560' },
  buttonSecondary: { backgroundColor: '#2d2d44' },
  buttonText: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
  navGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  navButton: {
    backgroundColor: '#1a1a2e', width: '48%', padding: 18, borderRadius: 10,
    alignItems: 'center', marginBottom: 10, borderWidth: 1, borderColor: '#2d2d44',
  },
  navButtonText: { color: '#4a90d9', fontSize: 14, fontWeight: '600' },
  disclaimer: {
    color: '#f59e0b', fontSize: 10, textAlign: 'center', marginTop: 8,
  },
});