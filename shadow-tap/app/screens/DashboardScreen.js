/**
 * DashboardScreen.js — ShadowTap main dashboard
 * Displays link status, PoE info, mode toggle, and connection controls.
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Switch, Alert
} from 'react-native';
import StatusIndicator from '../components/StatusIndicator';

export default function DashboardScreen({
  navigation, connected, status, onConnect, onDisconnect, bleManager
}) {
  const [mitmMode, setMitmMode] = useState(false);

  const toggleMode = async (value) => {
    if (!connected) return;
    try {
      await bleManager.setMode(value ? 1 : 0);
      setMitmMode(value);
    } catch (e) {
      Alert.alert('Error', 'Failed to change mode: ' + e.message);
    }
  };

  const ping = async () => {
    if (!connected) return;
    try {
      const pong = await bleManager.ping();
      Alert.alert('Ping', pong ? 'Device responded ✓' : 'No response ✗');
    } catch (e) {
      Alert.alert('Error', 'Ping failed: ' + e.message);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>⚡ ShadowTap</Text>
      <Text style={styles.subtitle}>PoE Stealth Network Tap & MITM</Text>

      {/* Connection Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CONNECTION</Text>
        <TouchableOpacity
          style={[styles.button, connected ? styles.buttonRed : styles.buttonGreen]}
          onPress={connected ? onDisconnect : onConnect}
        >
          <Text style={styles.buttonText}>
            {connected ? 'DISCONNECT' : 'SCAN & CONNECT'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Link Status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>LINK STATUS</Text>
        <View style={styles.statusRow}>
          <StatusIndicator
            label="UPLINK"
            active={status.uplinkLink}
            color="#00ff88"
          />
          <StatusIndicator
            label="TARGET"
            active={status.targetLink}
            color="#00aaff"
          />
        </View>
      </View>

      {/* Mode Toggle */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>MODE</Text>
        <View style={styles.modeRow}>
          <Text style={[styles.modeLabel, !mitmMode && styles.modeActive]}>
            TAP (Passive)
          </Text>
          <Switch
            value={mitmMode}
            onValueChange={toggleMode}
            trackColor={{ false: '#333', true: '#ff6600' }}
            thumbColor={mitmMode ? '#ffaa00' : '#888'}
            disabled={!connected}
          />
          <Text style={[styles.modeLabel, mitmMode && styles.modeActive]}>
            MITM (Active)
          </Text>
        </View>
      </View>

      {/* Quick Actions */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>QUICK ACTIONS</Text>
        <TouchableOpacity style={styles.actionButton} onPress={ping} disabled={!connected}>
          <Text style={styles.actionText}>📡 Ping Device</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('Rules')}
          disabled={!connected}
        >
          <Text style={styles.actionText}>🔧 MITM Rules ({status.ruleCount})</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('Capture')}
          disabled={!connected}
        >
          <Text style={styles.actionText}>💾 Packet Capture</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
    padding: 20,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#00ff88',
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 14,
    color: '#888',
    textAlign: 'center',
    marginBottom: 20,
  },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: {
    fontSize: 12,
    color: '#666',
    fontWeight: 'bold',
    marginBottom: 12,
    letterSpacing: 2,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  modeRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  modeLabel: {
    color: '#555',
    fontSize: 14,
  },
  modeActive: {
    color: '#00ff88',
    fontWeight: 'bold',
  },
  button: {
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
  },
  buttonGreen: {
    backgroundColor: '#00aa55',
  },
  buttonRed: {
    backgroundColor: '#aa3333',
  },
  buttonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 16,
  },
  actionButton: {
    backgroundColor: '#252540',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  actionText: {
    color: '#ccc',
    fontSize: 16,
  },
});