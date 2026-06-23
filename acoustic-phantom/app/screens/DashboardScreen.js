/**
 * ACOUSTIC-PHANTOM — Dashboard Screen
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Main status screen: shows BLE connection state, device status,
 * battery, active profile, event count, and beam bearing. Provides
 * buttons for profile selection, arm/disarm, and BLE scan/connect.
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert,
} from 'react-native';
import { useBLE } from '../utils/ble';
import { CMD, PROFILES, STATES } from '../utils/protocol';
import BeamIndicator from '../components/BeamIndicator';

export default function DashboardScreen() {
  const ble = useBLE();
  const [selectedProfile, setSelectedProfile] = useState(0);

  // Request status every 2 seconds when connected
  useEffect(() => {
    if (!ble.connected) return;
    const interval = setInterval(() => {
      ble.sendCommand(CMD.GET_STATUS);
    }, 2000);
    return () => clearInterval(interval);
  }, [ble.connected]);

  const handleScan = () => ble.scan();
  const handleConnect = () => ble.connect();
  const handleDisconnect = () => ble.disconnect();
  const handleArm = () => ble.sendCommand(CMD.ARM);
  const handleDisarm = () => ble.sendCommand(CMD.DISARM);

  const handleProfileSelect = (profileId) => {
    setSelectedProfile(profileId);
    ble.sendCommand(CMD.SET_PROFILE, [profileId]);
  };

  const handleBeamSteer = (angle) => {
    const data = new Uint8Array(2);
    data[0] = angle & 0xFF;
    data[1] = (angle >> 8) & 0xFF;
    ble.sendCommand(CMD.SET_BEAM, Array.from(data));
  };

  const state = ble.status?.state ?? 0;
  const stateName = STATES[state] || 'UNKNOWN';
  const battery = ble.status?.battery ?? 0;
  const eventCount = ble.status?.eventCount ?? 0;
  const profile = ble.status?.profile ?? selectedProfile;

  // Battery color
  const batteryColor = battery > 50 ? '#4CAF50' : battery > 20 ? '#FF9800' : '#F44336';

  // State color
  const stateColor = state === 2 ? '#4CAF50' : state === 1 ? '#FF9800' :
                     state === 5 ? '#F44336' : '#2196F3';

  return (
    <ScrollView style={styles.container}>
      {/* Connection status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        {!ble.connected ? (
          <View style={styles.buttonRow}>
            <TouchableOpacity
              style={[styles.button, styles.primaryButton]}
              onPress={handleScan}
              disabled={ble.scanning}
            >
              <Text style={styles.buttonText}>
                {ble.scanning ? 'Scanning...' : 'Scan for Device'}
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.button, styles.secondaryButton]}
              onPress={handleConnect}
              disabled={!ble.status}
            >
              <Text style={styles.buttonText}>Connect</Text>
            </TouchableOpacity>
          </View>
        ) : (
          <View style={styles.buttonRow}>
            <View style={styles.connectedIndicator}>
              <Text style={styles.connectedText}>✓ Connected</Text>
            </View>
            <TouchableOpacity
              style={[styles.button, styles.dangerButton]}
              onPress={handleDisconnect}
            >
              <Text style={styles.buttonText}>Disconnect</Text>
            </TouchableOpacity>
          </View>
        )}
        {ble.error && <Text style={styles.errorText}>Error: {ble.error}</Text>}
      </View>

      {/* Device status */}
      {ble.connected && (
        <>
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Device Status</Text>
            <View style={styles.statusGrid}>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>State</Text>
                <Text style={[styles.statusValue, { color: stateColor }]}>
                  {stateName}
                </Text>
              </View>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Battery</Text>
                <Text style={[styles.statusValue, { color: batteryColor }]}>
                  {battery}%
                </Text>
              </View>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Events</Text>
                <Text style={styles.statusValue}>{eventCount}</Text>
              </View>
              <View style={styles.statusItem}>
                <Text style={styles.statusLabel}>Profile</Text>
                <Text style={styles.statusValue}>
                  {PROFILES[profile]?.name ?? 'N/A'}
                </Text>
              </View>
            </View>
          </View>

          {/* Beam indicator */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Beam Direction</Text>
            <BeamIndicator
              bearing={0}
              onSteer={handleBeamSteer}
            />
          </View>

          {/* Profile selection */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Attack Profile</Text>
            {PROFILES.map(p => (
              <TouchableOpacity
                key={p.id}
                style={[
                  styles.profileButton,
                  profile === p.id && styles.profileButtonActive,
                ]}
                onPress={() => handleProfileSelect(p.id)}
              >
                <Text style={[
                  styles.profileButtonText,
                  profile === p.id && styles.profileButtonTextActive,
                ]}>
                  {p.name}
                </Text>
              </TouchableOpacity>
            ))}
          </View>

          {/* Arm/Disarm controls */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Capture Control</Text>
            <View style={styles.buttonRow}>
              {state < 2 ? (
                <TouchableOpacity
                  style={[styles.button, styles.armButton]}
                  onPress={handleArm}
                >
                  <Text style={styles.buttonText}>▶ ARM</Text>
                </TouchableOpacity>
              ) : (
                <TouchableOpacity
                  style={[styles.button, styles.dangerButton]}
                  onPress={handleDisarm}
                >
                  <Text style={styles.buttonText}>■ STOP</Text>
                </TouchableOpacity>
              )}
            </View>
          </View>
        </>
      )}

      <View style={styles.footer}>
        <Text style={styles.footerText}>ACOUSTIC-PHANTOM v1.0.0</Text>
        <Text style={styles.footerText}>Author: jayis1</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a1a' },
  section: {
    padding: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  sectionTitle: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#888',
    marginBottom: 12,
    textTransform: 'uppercase',
  },
  buttonRow: { flexDirection: 'row', gap: 12 },
  button: {
    flex: 1,
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  primaryButton: { backgroundColor: '#2196F3' },
  secondaryButton: { backgroundColor: '#555' },
  armButton: { backgroundColor: '#4CAF50' },
  dangerButton: { backgroundColor: '#F44336' },
  buttonText: { color: '#FFF', fontSize: 16, fontWeight: '600' },
  connectedIndicator: {
    flex: 1,
    padding: 14,
    borderRadius: 8,
    backgroundColor: '#1B5E20',
    alignItems: 'center',
  },
  connectedText: { color: '#4CAF50', fontSize: 16, fontWeight: '600' },
  errorText: { color: '#F44336', fontSize: 12, marginTop: 8 },
  statusGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 12 },
  statusItem: {
    flex: 1,
    minWidth: '45%',
    backgroundColor: '#222',
    padding: 12,
    borderRadius: 8,
  },
  statusLabel: { color: '#888', fontSize: 12, marginBottom: 4 },
  statusValue: { color: '#FFF', fontSize: 20, fontWeight: 'bold' },
  profileButton: {
    padding: 12,
    marginBottom: 8,
    borderRadius: 8,
    backgroundColor: '#333',
  },
  profileButtonActive: { backgroundColor: '#2196F3' },
  profileButtonText: { color: '#CCC', fontSize: 16 },
  profileButtonTextActive: { color: '#FFF', fontWeight: 'bold' },
  footer: { padding: 20, alignItems: 'center' },
  footerText: { color: '#555', fontSize: 12 },
});