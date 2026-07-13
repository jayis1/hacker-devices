/**
 * eddy-phantom — DashboardScreen.js
 * Main dashboard showing connection status, device state, battery,
 * active profile, and quick controls.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Alert,
  SafeAreaView, StatusBar, ActivityIndicator,
} from 'react-native';

export default function DashboardScreen({ navigation, bleManager, connected,
  deviceState, battery, profile, keystrokes }) {
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
    if (deviceState === 'IDLE') {
      await bleManager.arm();
    } else if (deviceState === 'ARMED') {
      await bleManager.disarm();
    }
  };

  const stateColor = {
    'IDLE': '#4a90d9',
    'ARMED': '#e94560',
    'CAPTURE': '#ff6b6b',
    'CALIBRATE': '#f0a500',
    'BOOT': '#888',
    'ERROR': '#e94560',
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>EDDY-PHANTOM</Text>
        <Text style={styles.subtitle}>EM Emanation Keylogger</Text>
      </View>

      {/* Connection status card */}
      <View style={styles.card}>
        <View style={styles.statusRow}>
          <View style={[styles.statusDot,
            { backgroundColor: connected ? '#4ade80' : '#6b7280' }]} />
          <Text style={styles.statusText}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>

        {!connected ? (
          <TouchableOpacity
            style={styles.connectButton}
            onPress={handleConnect}
            disabled={scanning}
          >
            {scanning ? (
              <ActivityIndicator color="#fff" />
            ) : (
              <Text style={styles.buttonText}>Scan & Connect</Text>
            )}
          </TouchableOpacity>
        ) : (
          <View style={styles.infoGrid}>
            <InfoRow label="State" value={deviceState}
              color={stateColor[deviceState] || '#fff'} />
            <InfoRow label="Battery" value={`${battery}%`} />
            <InfoRow label="Profile" value={profile || 'None'} />
            <InfoRow label="Captured" value={`${keystrokes.length} keys`} />
          </View>
        )}
      </View>

      {/* Quick controls */}
      {connected && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Quick Controls</Text>
          <TouchableOpacity
            style={[styles.actionButton,
              { backgroundColor: deviceState === 'ARMED' ? '#e94560' : '#4a90d9' }]}
            onPress={handleArm}
          >
            <Text style={styles.buttonText}>
              {deviceState === 'ARMED' ? 'DISARM' : 'ARM'}
            </Text>
          </TouchableOpacity>
        </View>
      )}

      {/* Navigation cards */}
      <View style={styles.navGrid}>
        <NavCard title="Live Feed" icon="⚡"
          onPress={() => navigation.navigate('Live Feed')} />
        <NavCard title="Session Log" icon="📋"
          onPress={() => navigation.navigate('Session Log')} />
        <NavCard title="Profiles" icon="🎯"
          onPress={() => navigation.navigate('Profiles')} />
        <NavCard title="Raw Capture" icon="📊"
          onPress={() => navigation.navigate('Raw Capture')} />
        <NavCard title="Settings" icon="⚙️"
          onPress={() => navigation.navigate('Settings')} />
      </View>

      {/* Legal notice */}
      <View style={styles.legalNotice}>
        <Text style={styles.legalText}>
          ⚠ For authorized security research only. Author: jayis1
        </Text>
      </View>
    </SafeAreaView>
  );
}

function InfoRow({ label, value, color }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={[styles.infoValue, color ? { color } : null]}>{value}</Text>
    </View>
  );
}

function NavCard({ title, icon, onPress }) {
  return (
    <TouchableOpacity style={styles.navCard} onPress={onPress}>
      <Text style={styles.navIcon}>{icon}</Text>
      <Text style={styles.navTitle}>{title}</Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  header: { alignItems: 'center', marginBottom: 20, marginTop: 10 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#e94560' },
  subtitle: { fontSize: 12, color: '#888', marginTop: 4 },
  card: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16,
    marginBottom: 12, borderWidth: 1, borderColor: '#2a2a3e',
  },
  statusRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  statusDot: { width: 10, height: 10, borderRadius: 5, marginRight: 8 },
  statusText: { color: '#fff', fontSize: 16 },
  connectButton: {
    backgroundColor: '#4a90d9', padding: 14, borderRadius: 8,
    alignItems: 'center', marginTop: 8,
  },
  buttonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  infoGrid: { gap: 8 },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between' },
  infoLabel: { color: '#888', fontSize: 14 },
  infoValue: { color: '#fff', fontSize: 14, fontWeight: '600' },
  cardTitle: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  actionButton: {
    padding: 16, borderRadius: 8, alignItems: 'center',
  },
  navGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  navCard: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 20,
    width: '48%', alignItems: 'center', borderWidth: 1, borderColor: '#2a2a3e',
  },
  navIcon: { fontSize: 28, marginBottom: 8 },
  navTitle: { color: '#fff', fontSize: 14, fontWeight: '600' },
  legalNotice: { marginTop: 'auto', padding: 12, alignItems: 'center' },
  legalText: { color: '#666', fontSize: 10 },
});