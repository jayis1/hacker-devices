/*
 * screens/TrajectoryScreen.js — Trajectory and position simulation screen
 *
 * Allows setting the spoofed geographic position (lat/lon/alt) and GPS time
 * (week + TOW).  Includes preset locations and a velocity input for dynamic
 * trajectory simulation.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity, ScrollView, Alert,
} from 'react-native';
import { bleManager } from '../utils/ble';
import { cmdSetTrajectory, cmdSetTime } from '../utils/protocol';

const PRESETS = [
  { name: 'San Francisco', lat: 37.7749,   lon: -122.4194, alt: 0 },
  { name: 'New York',      lat: 40.7128,   lon: -74.0060,  alt: 10 },
  { name: 'London',         lat: 51.5074,   lon: -0.1278,   alt: 35 },
  { name: 'Tokyo',          lat: 35.6762,   lon: 139.6503,  alt: 40 },
  { name: 'Null Island',    lat: 0.0,       lon: 0.0,        alt: 0 },
  { name: 'North Pole',     lat: 89.99,     lon: 0.0,        alt: 0 },
];

export default function TrajectoryScreen() {
  const [lat, setLat] = useState('37.7749');
  const [lon, setLon] = useState('-122.4194');
  const [alt, setAlt] = useState('0');
  const [week, setWeek] = useState('2300');
  const [tow, setTow] = useState('0');

  const applyPosition = async () => {
    const latVal = parseFloat(lat);
    const lonVal = parseFloat(lon);
    const altVal = parseFloat(alt);
    if (isNaN(latVal) || isNaN(lonVal) || isNaN(altVal)) {
      Alert.alert('Invalid Input', 'Coordinates must be numeric');
      return;
    }
    await bleManager.send(cmdSetTrajectory(latVal, lonVal, altVal));
    Alert.alert('Position Set', `${latVal}, ${lonVal} @ ${altVal}m`);
  };

  const applyTime = async () => {
    const weekVal = parseInt(week, 10);
    const towVal = parseInt(tow, 10);
    if (isNaN(weekVal) || isNaN(towVal)) {
      Alert.alert('Invalid Input', 'Week and TOW must be integers');
      return;
    }
    await bleManager.send(cmdSetTime(weekVal, towVal));
    Alert.alert('Time Set', `Week ${weekVal}, TOW ${towVal}ms`);
  };

  const applyPreset = (preset) => {
    setLat(preset.lat.toString());
    setLon(preset.lon.toString());
    setAlt(preset.alt.toString());
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Trajectory & Position</Text>
      </View>

      {/* Position input */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>SPOOFED POSITION</Text>

        <Text style={styles.inputLabel}>Latitude (deg)</Text>
        <TextInput
          style={styles.input}
          keyboardType="numeric"
          value={lat}
          onChangeText={setLat}
          placeholder="37.7749"
          placeholderTextColor="#555"
        />

        <Text style={styles.inputLabel}>Longitude (deg)</Text>
        <TextInput
          style={styles.input}
          keyboardType="numeric"
          value={lon}
          onChangeText={setLon}
          placeholder="-122.4194"
          placeholderTextColor="#555"
        />

        <Text style={styles.inputLabel}>Altitude (m)</Text>
        <TextInput
          style={styles.input}
          keyboardType="numeric"
          value={alt}
          onChangeText={setAlt}
          placeholder="0"
          placeholderTextColor="#555"
        />

        <TouchableOpacity style={styles.applyBtn} onPress={applyPosition}>
          <Text style={styles.applyBtnText}>Apply Position</Text>
        </TouchableOpacity>
      </View>

      {/* Presets */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>LOCATION PRESETS</Text>
        {PRESETS.map(p => (
          <TouchableOpacity
            key={p.name}
            style={styles.presetBtn}
            onPress={() => applyPreset(p)}
          >
            <Text style={styles.presetName}>{p.name}</Text>
            <Text style={styles.presetCoords}>
              {p.lat.toFixed(4)}, {p.lon.toFixed(4)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* GPS Time */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>GPS TIME</Text>

        <Text style={styles.inputLabel}>GPS Week</Text>
        <TextInput
          style={styles.input}
          keyboardType="numeric"
          value={week}
          onChangeText={setWeek}
          placeholder="2300"
          placeholderTextColor="#555"
        />

        <Text style={styles.inputLabel}>Time of Week (ms)</Text>
        <TextInput
          style={styles.input}
          keyboardType="numeric"
          value={tow}
          onChangeText={setTow}
          placeholder="0"
          placeholderTextColor="#555"
        />

        <TouchableOpacity style={[styles.applyBtn, { backgroundColor: '#3b82f6' }]} onPress={applyTime}>
          <Text style={styles.applyBtnText}>Apply Time</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.disclaimer}>
        <Text style={styles.disclaimerText}>
          ⚠ Setting a spoofed position affects all GNSS receivers{'\n'}
          in range. Use only in authorized shielded environments.{'\n'}
          © jayis1
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#3b82f6', fontSize: 20, fontWeight: 'bold', fontFamily: 'monospace' },
  section: { margin: 10, padding: 15, backgroundColor: '#1a1a2e', borderRadius: 8 },
  sectionTitle: { color: '#3b82f6', fontSize: 11, fontWeight: 'bold', marginBottom: 10, letterSpacing: 1 },
  inputLabel: { color: '#888', fontSize: 12, marginTop: 10, marginBottom: 5 },
  input: { backgroundColor: '#2a2a3e', color: '#fff', borderRadius: 6, padding: 10, fontSize: 14, fontFamily: 'monospace' },
  applyBtn: { marginTop: 15, padding: 12, backgroundColor: '#22c55e', borderRadius: 8, alignItems: 'center' },
  applyBtnText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  presetBtn: { flexDirection: 'row', justifyContent: 'space-between', padding: 10, backgroundColor: '#2a2a3e', borderRadius: 6, marginVertical: 4 },
  presetName: { color: '#e0e0e0', fontSize: 14 },
  presetCoords: { color: '#3b82f6', fontSize: 11, fontFamily: 'monospace' },
  disclaimer: { margin: 10, padding: 15, backgroundColor: '#2a0a0a', borderRadius: 8, borderWidth: 1, borderColor: '#ef4444' },
  disclaimerText: { color: '#fbbf24', fontSize: 10, textAlign: 'center', lineHeight: 16, fontFamily: 'monospace' },
});