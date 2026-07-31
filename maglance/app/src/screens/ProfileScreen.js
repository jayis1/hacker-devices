/**
 * ProfileScreen.js — Profile management for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Create, edit, save, and load attack profiles. Each profile stores
 * mode, polarity, pulse width, current, count, delay, sweep parameters,
 * and a human-readable name. 16 slots available (0-15).
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, Alert, ScrollView, FlatList } from 'react-native';
import { useBLE } from '../context/BLEContext';

export default function ProfileScreen() {
  const { connectionState, sendCommand, telemetry } = useBLE();

  const [profiles, setProfiles] = useState([]);
  const [selectedSlot, setSelectedSlot] = useState(0);
  const [profileName, setProfileName] = useState('');
  const [editMode, setEditMode] = useState(false);

  // Editable profile parameters
  const [mode, setMode] = useState('1');        // PULSE
  const [polarity, setPolarity] = useState('0'); // NORTH
  const [widthNs, setWidthNs] = useState('100000');
  const [currentMa, setCurrentMa] = useState('5000');
  const [count, setCount] = useState('1');
  const [delayUs, setDelayUs] = useState('1000');
  const [sweepStart, setSweepStart] = useState('1000');
  const [sweepStop, setSweepStop] = useState('100000');
  const [sweepSteps, setSweepSteps] = useState('50');
  const [sweepDwell, setSweepDwell] = useState('100');

  const modeNames = ['IDLE', 'PULSE', 'DC', 'SWEEP', 'SENSE', 'PROFILE', 'CAL', 'ERROR'];

  // Scan all 16 slots on mount
  useEffect(() => {
    if (connectionState === 'connected') {
      scanProfiles();
    }
  }, [connectionState]);

  const scanProfiles = async () => {
    const found = [];
    for (let i = 0; i < 16; i++) {
      // In a real implementation, we'd send PROFILE GET and parse the response
      // For now, just show the slot structure
      found.push({ slot: i, name: `Slot ${i}`, occupied: false });
    }
    setProfiles(found);
  };

  const handleLoad = (slot) => {
    Alert.alert(
      'Load Profile',
      `Load profile from slot ${slot}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Load',
          onPress: () => {
            sendCommand(`PROFILE LOAD ${slot}\n`);
            setSelectedSlot(slot);
          }
        },
      ]
    );
  };

  const handleSave = () => {
    if (!profileName.trim()) {
      Alert.alert('No Name', 'Please enter a profile name');
      return;
    }

    Alert.alert(
      'Save Profile',
      `Save "${profileName}" to slot ${selectedSlot}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Save',
          onPress: () => {
            // First set the current parameters, then save
            // The device stores the current system status as a profile
            sendCommand(`PROFILE SAVE ${selectedSlot} ${profileName}\n`);
            Alert.alert('Saved', `Profile saved to slot ${selectedSlot}`);
            setEditMode(false);
          }
        },
      ]
    );
  };

  const handleErase = (slot) => {
    Alert.alert(
      'Erase Profile',
      `Erase profile in slot ${slot}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Erase',
          style: 'destructive',
          onPress: () => {
            // Send erase command (custom extension)
            // Not in the standard protocol, but would be:
            // sendCommand(`PROFILE ERASE ${slot}\n`);
            Alert.alert('Not Implemented', 'Erase requires firmware extension');
          }
        },
      ]
    );
  };

  const renderProfileItem = ({ item }) => (
    <TouchableOpacity
      style={[
        styles.profileItem,
        item.slot === selectedSlot && styles.profileItemSelected
      ]}
      onPress={() => handleLoad(item.slot)}
      onLongPress={() => handleErase(item.slot)}
    >
      <View style={styles.profileItemLeft}>
        <Text style={styles.profileSlot}>#{item.slot}</Text>
        <Text style={styles.profileName}>{item.name}</Text>
      </View>
      <Text style={styles.profileStatus}>
        {item.occupied ? '●' : '○'}
      </Text>
    </TouchableOpacity>
  );

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Profiles</Text>
      <Text style={styles.subtitle}>Manage 16 attack profile slots</Text>

      {/* Profile list */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Stored Profiles</Text>
        <FlatList
          data={profiles}
          renderItem={renderProfileItem}
          keyExtractor={item => item.slot.toString()}
          scrollEnabled={false}
        />
      </View>

      {/* Edit / Create */}
      {editMode ? (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>
            {`Edit Profile — Slot ${selectedSlot}`}
          </Text>

          <View style={styles.inputGroup}>
            <Text style={styles.label}>Profile Name</Text>
            <TextInput style={styles.input} value={profileName}
              onChangeText={setProfileName} placeholder="e.g. MRAM-flip-5A" />
          </View>

          <View style={styles.inputRow}>
            <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
              <Text style={styles.label}>Mode</Text>
              <TextInput style={styles.input} value={mode} onChangeText={setMode}
                keyboardType="numeric" placeholder="0-7" />
              <Text style={styles.hint}>{modeNames[parseInt(mode) || 0]}</Text>
            </View>
            <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
              <Text style={styles.label}>Polarity</Text>
              <TextInput style={styles.input} value={polarity} onChangeText={setPolarity}
                keyboardType="numeric" placeholder="0=N, 1=S" />
            </View>
          </View>

          <View style={styles.inputRow}>
            <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
              <Text style={styles.label}>Width (ns)</Text>
              <TextInput style={styles.input} value={widthNs} onChangeText={setWidthNs}
                keyboardType="numeric" />
            </View>
            <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
              <Text style={styles.label}>Current (mA)</Text>
              <TextInput style={styles.input} value={currentMa} onChangeText={setCurrentMa}
                keyboardType="numeric" />
            </View>
          </View>

          <View style={styles.inputRow}>
            <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
              <Text style={styles.label}>Count</Text>
              <TextInput style={styles.input} value={count} onChangeText={setCount}
                keyboardType="numeric" />
            </View>
            <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
              <Text style={styles.label}>Delay (µs)</Text>
              <TextInput style={styles.input} value={delayUs} onChangeText={setDelayUs}
                keyboardType="numeric" />
            </View>
          </View>

          {/* Sweep parameters (for SWEEP mode) */}
          <Text style={styles.subsectionTitle}>Sweep Parameters</Text>
          <View style={styles.inputRow}>
            <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
              <Text style={styles.label}>Start (Hz)</Text>
              <TextInput style={styles.input} value={sweepStart} onChangeText={setSweepStart}
                keyboardType="numeric" />
            </View>
            <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
              <Text style={styles.label}>Stop (Hz)</Text>
              <TextInput style={styles.input} value={sweepStop} onChangeText={setSweepStop}
                keyboardType="numeric" />
            </View>
          </View>
          <View style={styles.inputRow}>
            <View style={[styles.inputGroup, { flex: 1, marginRight: 8 }]}>
              <Text style={styles.label}>Steps</Text>
              <TextInput style={styles.input} value={sweepSteps} onChangeText={setSweepSteps}
                keyboardType="numeric" />
            </View>
            <View style={[styles.inputGroup, { flex: 1, marginLeft: 8 }]}>
              <Text style={styles.label}>Dwell (ms)</Text>
              <TextInput style={styles.input} value={sweepDwell} onChangeText={setSweepDwell}
                keyboardType="numeric" />
            </View>
          </View>

          <TouchableOpacity style={styles.saveButton} onPress={handleSave}>
            <Text style={styles.saveButtonText}>💾 SAVE PROFILE</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.cancelButton} onPress={() => setEditMode(false)}>
            <Text style={styles.cancelButtonText}>Cancel</Text>
          </TouchableOpacity>
        </View>
      ) : (
        <TouchableOpacity style={styles.newButton} onPress={() => {
          setEditMode(true);
          setProfileName(`Profile-${selectedSlot}`);
        }}>
          <Text style={styles.newButtonText}>+ New Profile</Text>
        </TouchableOpacity>
      )}

      {/* Calibration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Coil Calibration</Text>
        <Text style={styles.calText}>
          Run calibration to characterize the coil-to-field transfer function.
          This ensures accurate field strength targeting.
        </Text>
        <TouchableOpacity style={styles.calButton} onPress={() => sendCommand('CAL 1\n')}>
          <Text style={styles.calButtonText}>▶ Run Calibration</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#e74c3c', marginBottom: 4 },
  subtitle: { color: '#95a5a6', fontSize: 12, marginBottom: 16 },
  section: { backgroundColor: '#16213e', borderRadius: 8, padding: 12, marginBottom: 12 },
  sectionTitle: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  subsectionTitle: { color: '#3498db', fontSize: 13, fontWeight: 'bold', marginTop: 8, marginBottom: 8 },
  profileItem: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    backgroundColor: '#1a1a2e', padding: 12, borderRadius: 6, marginBottom: 4 },
  profileItemSelected: { borderWidth: 2, borderColor: '#e74c3c' },
  profileItemLeft: { flexDirection: 'row', alignItems: 'center' },
  profileSlot: { color: '#e74c3c', fontSize: 14, fontWeight: 'bold', marginRight: 12, fontFamily: 'monospace' },
  profileName: { color: '#ecf0f1', fontSize: 14 },
  profileStatus: { color: '#95a5a6', fontSize: 16 },
  inputGroup: { marginBottom: 12 },
  inputRow: { flexDirection: 'row' },
  label: { color: '#ecf0f1', fontSize: 12, fontWeight: 'bold', marginBottom: 4 },
  input: { backgroundColor: '#1a1a2e', color: '#ffffff', borderWidth: 1,
           borderColor: '#34495e', borderRadius: 6, padding: 10, fontSize: 14 },
  hint: { color: '#7f8c8d', fontSize: 10, marginTop: 2 },
  newButton: { backgroundColor: '#2980b9', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 12 },
  newButtonText: { color: '#ffffff', fontSize: 16, fontWeight: 'bold' },
  saveButton: { backgroundColor: '#27ae60', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  saveButtonText: { color: '#ffffff', fontSize: 16, fontWeight: 'bold' },
  cancelButton: { backgroundColor: '#34495e', padding: 12, borderRadius: 8, alignItems: 'center' },
  cancelButtonText: { color: '#ffffff', fontSize: 14 },
  calButton: { backgroundColor: '#8e44ad', padding: 12, borderRadius: 8, alignItems: 'center', marginTop: 8 },
  calButtonText: { color: '#ffffff', fontSize: 14, fontWeight: 'bold' },
  calText: { color: '#95a5a6', fontSize: 12, marginBottom: 8, lineHeight: 18 },
});