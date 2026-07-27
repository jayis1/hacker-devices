// src/screens/DashboardScreen.tsx — Device status dashboard
//
// Author: jayis1
// License: GPL-2.0

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import { useBle } from '../ble/BleManager';
import StatusCard from '../components/StatusCard';
import { CMD } from '../types';

export default function DashboardScreen() {
  const { connected, scanning, deviceName, status, connect, disconnect, sendCommand, error } = useBle();
  const [tick, setTick] = useState(0);

  useEffect(() => {
    const id = setInterval(() => setTick((t) => t + 1), 1000);
    return () => clearInterval(id);
  }, []);

  const handleConnect = async () => {
    if (connected) {
      await disconnect();
    } else {
      await connect();
    }
  };

  const handlePing = async () => {
    if (connected) {
      await sendCommand(CMD.PING, new Uint8Array(0));
      Alert.alert('Ping', 'Sent ping to device');
    }
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <View style={styles.connectionCard}>
        <Text style={styles.connectionTitle}>
          {connected ? '🟢 Connected' : scanning ? '🔵 Scanning...' : '🔴 Disconnected'}
        </Text>
        {deviceName && <Text style={styles.deviceName}>{deviceName}</Text>}
        {error && <Text style={styles.errorText}>{error}</Text>}
        <TouchableOpacity
          style={[styles.button, connected ? styles.buttonDanger : styles.buttonPrimary]}
          onPress={handleConnect}
        >
          <Text style={styles.buttonText}>
            {connected ? 'Disconnect' : scanning ? 'Scanning...' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>

      {connected && status && (
        <>
          <Text style={styles.sectionTitle}>System Status</Text>
          <StatusCard label="Operating Mode" value={status.mode.toUpperCase()} />
          <StatusCard label="Skew Active" value={status.skewActive ? 'YES ⚠️' : 'no'} />
          <StatusCard label="Skew Profile" value={status.skewProfile} />
          <StatusCard label="Current Skew" value={`${status.currentSkewNsStr} ns`} />
          <StatusCard label="Frames Captured" value={status.framesCaptured.toLocaleString()} />
          <StatusCard label="Frames Modified" value={status.framesModified.toLocaleString()} />
          <StatusCard label="NTP Requests" value={status.ntpRequests.toString()} />
          <StatusCard label="NTP Responses" value={status.ntpResponses.toString()} />

          <Text style={styles.sectionTitle}>PTP Grandmaster</Text>
          <StatusCard label="Observed GM Priority1" value={status.observedGmPriority1.toString()} />
          <StatusCard label="Observed GM Class" value={status.observedGmClockClass.toString()} />
          <StatusCard label="Spoofed GM Priority1" value={status.spoofedGmPriority1.toString()} />
          <StatusCard label="Spoofed GM Class" value={status.spoofedGmClockClass.toString()} />

          <TouchableOpacity style={styles.buttonSecondary} onPress={handlePing}>
            <Text style={styles.buttonText}>Ping Device</Text>
          </TouchableOpacity>
        </>
      )}

      {!connected && (
        <View style={styles.placeholder}>
          <Text style={styles.placeholderText}>Connect to a Chronos-Phantom device to begin.</Text>
          <Text style={styles.placeholderHint}>
            The device advertises as "Chronos-Phantom-XXXX" over BLE.
            Tap Connect to start scanning.
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a' },
  content: { padding: 16, paddingBottom: 40 },
  connectionCard: {
    backgroundColor: '#1a1a1a',
    borderRadius: 12,
    padding: 20,
    alignItems: 'center',
    marginBottom: 16,
  },
  connectionTitle: { fontSize: 18, fontWeight: 'bold', color: '#00ff88' },
  deviceName: { fontSize: 14, color: '#888', marginTop: 4 },
  errorText: { fontSize: 12, color: '#ff4444', marginTop: 8 },
  button: { marginTop: 16, paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8 },
  buttonPrimary: { backgroundColor: '#00aa44' },
  buttonDanger: { backgroundColor: '#aa3333' },
  buttonSecondary: {
    backgroundColor: '#333', marginTop: 16, paddingHorizontal: 24,
    paddingVertical: 12, borderRadius: 8, alignSelf: 'center',
  },
  buttonText: { color: 'white', fontSize: 14, fontWeight: '600' },
  sectionTitle: { fontSize: 16, color: '#00ff88', marginTop: 24, marginBottom: 8, fontWeight: '600' },
  placeholder: { alignItems: 'center', marginTop: 40 },
  placeholderText: { color: '#666', fontSize: 16, textAlign: 'center' },
  placeholderHint: { color: '#444', fontSize: 12, marginTop: 8, textAlign: 'center' },
});

// Author: jayis1
// License: GPL-2.0