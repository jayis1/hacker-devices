/**
 * eddy-phantom — ProfileScreen.js
 * Keyboard profile selection, calibration workflow, and
 * profile information display.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, FlatList, SafeAreaView,
  StatusBar, TouchableOpacity, Alert, TextInput,
} from 'react-native';

// Built-in profile catalog
const BUILTIN_PROFILES = [
  { id: 'holtek-ht82k629a', name: 'Holtek HT82K629A', ctrlId: 0x629A,
    desc: 'Common in low-cost USB keyboards', keys: 104 },
  { id: 'cypress-cy7c63743', name: 'Cypress CY7C63743', ctrlId: 0x6374,
    desc: 'Used in many office keyboards', keys: 104 },
  { id: 'nxp-lpc11u6x', name: 'NXP LPC11U6x', ctrlId: 0x11A6,
    desc: 'Mechanical keyboards, QMK variants', keys: 110 },
  { id: 'asix-ax88772', name: 'ASIX AX88772', ctrlId: 0x8772,
    desc: 'USB hub passthrough keyboards', keys: 104 },
  { id: 'microchip-usb2514', name: 'Microchip USB2514', ctrlId: 0x2514,
    desc: 'Hub-based keyboards', keys: 104 },
  { id: 'generic-8051', name: 'Generic 8051', ctrlId: 0x8051,
    desc: '8051-based keyboard controllers', keys: 104 },
];

export default function ProfileScreen({ bleManager, connected, profile }) {
  const [selectedProfile, setSelectedProfile] = useState(null);
  const [calibrating, setCalibrating] = useState(false);
  const [calKeyIdx, setCalKeyIdx] = useState(0);
  const [customCtrlId, setCustomCtrlId] = useState('');

  // Calibration key sequence (standard keyboard layout)
  const CAL_KEYS = [
    { scan: 0x04, label: 'A' }, { scan: 0x05, label: 'B' },
    { scan: 0x06, label: 'C' }, { scan: 0x07, label: 'D' },
    { scan: 0x08, label: 'E' }, { scan: 0x09, label: 'F' },
    { scan: 0x0A, label: 'G' }, { scan: 0x0B, label: 'H' },
    { scan: 0x0C, label: 'I' }, { scan: 0x0D, label: 'J' },
    { scan: 0x0E, label: 'K' }, { scan: 0x0F, label: 'L' },
    { scan: 0x10, label: 'M' }, { scan: 0x11, label: 'N' },
    { scan: 0x12, label: 'O' }, { scan: 0x13, label: 'P' },
    { scan: 0x14, label: 'Q' }, { scan: 0x15, label: 'R' },
    { scan: 0x16, label: 'S' }, { scan: 0x17, label: 'T' },
    { scan: 0x18, label: 'U' }, { scan: 0x19, label: 'V' },
    { scan: 0x1A, label: 'W' }, { scan: 0x1B, label: 'X' },
    { scan: 0x1C, label: 'Y' }, { scan: 0x1D, label: 'Z' },
    { scan: 0x1E, label: '1' }, { scan: 0x1F, label: '2' },
    { scan: 0x20, label: '3' }, { scan: 0x21, label: '4' },
    { scan: 0x22, label: '5' }, { scan: 0x23, label: '6' },
    { scan: 0x24, label: '7' }, { scan: 0x25, label: '8' },
    { scan: 0x26, label: '9' }, { scan: 0x27, label: '0' },
    { scan: 0x28, label: 'Enter' }, { scan: 0x29, label: 'ESC' },
    { scan: 0x2A, label: 'Backspace' }, { scan: 0x2B, label: 'Tab' },
    { scan: 0x2C, label: 'Space' },
  ];

  const handleSelectProfile = async (profileId) => {
    setSelectedProfile(profileId);
    if (connected) {
      try {
        await bleManager.setProfile(profileId);
        Alert.alert('Profile Loaded', `Active profile: ${profileId}`);
      } catch (e) {
        Alert.alert('Error', 'Failed to load profile');
      }
    }
  };

  const handleStartCalibration = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to device first');
      return;
    }
    const ctrlId = customCtrlId ?
      parseInt(customCtrlId, 16) : 0x629A;
    try {
      await bleManager.startCalibration(ctrlId);
      setCalibrating(true);
      setCalKeyIdx(0);
      Alert.alert('Calibration Started',
        `Press each key when prompted. Start with: ${CAL_KEYS[0].label}`);
    } catch (e) {
      Alert.alert('Error', 'Failed to start calibration');
    }
  };

  const handleCalKey = async () => {
    if (calKeyIdx >= CAL_KEYS.length) {
      await handleFinishCalibration();
      return;
    }
    const key = CAL_KEYS[calKeyIdx];
    try {
      await bleManager.calibrateKey(key.scan);
      const nextIdx = calKeyIdx + 1;
      setCalKeyIdx(nextIdx);
      if (nextIdx >= CAL_KEYS.length) {
        Alert.alert('Calibration Complete',
          'All keys calibrated. Finishing...');
        await handleFinishCalibration();
      }
    } catch (e) {
      Alert.alert('Error', 'Failed to record calibration key');
    }
  };

  const handleFinishCalibration = async () => {
    try {
      await bleManager.finishCalibration();
      setCalibrating(false);
      setCalKeyIdx(0);
      Alert.alert('Success', 'Profile saved to device');
    } catch (e) {
      Alert.alert('Error', 'Failed to finish calibration');
    }
  };

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={[styles.profileCard,
        selectedProfile === item.id && styles.profileCardActive,
        profile === item.id && styles.profileCardCurrent]}
      onPress={() => handleSelectProfile(item.id)}
    >
      <View style={styles.profileHeader}>
        <Text style={styles.profileName}>{item.name}</Text>
        {profile === item.id && <Text style={styles.activeBadge}>ACTIVE</Text>}
      </View>
      <Text style={styles.profileDesc}>{item.desc}</Text>
      <Text style={styles.profileMeta}>
        Controller: 0x{item.ctrlId.toString(16).toUpperCase()} | {item.keys} keys
      </Text>
    </TouchableOpacity>
  );

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />

      <View style={styles.headerCard}>
        <Text style={styles.headerTitle}>Keyboard Profiles</Text>
        <Text style={styles.headerSubtitle}>
          Select a profile matching the target keyboard's controller.
          Current: {profile || 'None'}
        </Text>
      </View>

      {/* Calibration section */}
      <View style={styles.calCard}>
        <Text style={styles.calTitle}>Custom Calibration</Text>
        <Text style={styles.calDesc}>
          Train a new profile by pressing keys on an identical keyboard.
        </Text>
        <View style={styles.calInputRow}>
          <Text style={styles.calLabel}>Controller ID (hex):</Text>
          <TextInput
            style={styles.calInput}
            placeholder="0x629A"
            placeholderTextColor="#444"
            value={customCtrlId}
            onChangeText={setCustomCtrlId}
            autoCapitalize="characters"
          />
        </View>

        {!calibrating ? (
          <TouchableOpacity
            style={styles.calButton}
            onPress={handleStartCalibration}
          >
            <Text style={styles.calButtonText}>Start Calibration</Text>
          </TouchableOpacity>
        ) : (
          <View style={styles.calProgress}>
            <Text style={styles.calKeyLabel}>
              Press: {CAL_KEYS[calKeyIdx]?.label || 'Done'}
            </Text>
            <Text style={styles.calProgress}>
              Key {calKeyIdx + 1} of {CAL_KEYS.length}
            </Text>
            <View style={styles.calProgressBar}>
              <View style={[styles.calProgressFill, {
                width: `${(calKeyIdx / CAL_KEYS.length) * 100}%`,
              }]} />
            </View>
            <TouchableOpacity
              style={styles.calButton}
              onPress={handleCalKey}
            >
              <Text style={styles.calButtonText}>Record Key</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.calButton, { borderColor: '#e94560', marginTop: 8 }]}
              onPress={handleFinishCalibration}
            >
              <Text style={[styles.calButtonText, { color: '#e94560' }]}>
                Finish Early
              </Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      {/* Profile list */}
      <Text style={styles.sectionTitle}>Available Profiles</Text>
      <FlatList
        data={BUILTIN_PROFILES}
        renderItem={renderItem}
        keyExtractor={(item) => item.id}
        contentContainerStyle={{ paddingBottom: 20 }}
      />
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  headerCard: {
    backgroundColor: '#1a1a2e', margin: 12, borderRadius: 12, padding: 16,
    borderWidth: 1, borderColor: '#2a2a3e',
  },
  headerTitle: { color: '#e94560', fontSize: 18, fontWeight: 'bold' },
  headerSubtitle: { color: '#888', fontSize: 12, marginTop: 4 },
  calCard: {
    backgroundColor: '#1a1a2e', margin: 12, borderRadius: 12, padding: 16,
    borderWidth: 1, borderColor: '#2a2a3e',
  },
  calTitle: { color: '#fff', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  calDesc: { color: '#888', fontSize: 12, marginBottom: 12 },
  calInputRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  calLabel: { color: '#888', fontSize: 12, marginRight: 8 },
  calInput: {
    flex: 1, backgroundColor: '#0f0f1e', borderRadius: 6, padding: 8,
    color: '#fff', fontSize: 14, borderWidth: 1, borderColor: '#2a2a3e',
  },
  calButton: {
    backgroundColor: '#4a90d9', padding: 12, borderRadius: 8,
    alignItems: 'center', marginTop: 8,
  },
  calButtonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  calProgress: { alignItems: 'center' },
  calKeyLabel: { color: '#e94560', fontSize: 24, fontWeight: 'bold', marginVertical: 8 },
  calProgressBar: {
    width: '100%', height: 6, backgroundColor: '#2a2a3e', borderRadius: 3,
    marginVertical: 12, overflow: 'hidden',
  },
  calProgressFill: { height: '100%', backgroundColor: '#4a90d9', borderRadius: 3 },
  calProgress: { color: '#888', fontSize: 12 },
  sectionTitle: {
    color: '#888', fontSize: 14, fontWeight: 'bold',
    paddingHorizontal: 12, marginBottom: 8,
  },
  profileCard: {
    backgroundColor: '#1a1a2e', marginHorizontal: 12, borderRadius: 10,
    padding: 14, marginBottom: 8, borderWidth: 1, borderColor: '#2a2a3e',
  },
  profileCardActive: { borderColor: '#4a90d9', backgroundColor: '#4a90d922' },
  profileCardCurrent: { borderColor: '#e94560' },
  profileHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  profileName: { color: '#fff', fontSize: 15, fontWeight: '600' },
  activeBadge: {
    color: '#e94560', fontSize: 10, fontWeight: 'bold',
    backgroundColor: '#e9456022', padding: 4, borderRadius: 4,
  },
  profileDesc: { color: '#888', fontSize: 12, marginTop: 4 },
  profileMeta: { color: '#555', fontSize: 11, marginTop: 4, fontFamily: 'monospace' },
});