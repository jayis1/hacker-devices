/**
 * screens/DashboardScreen.js — Live device status
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { Navigation } from 'react-native-navigation';
import { MODE_NAMES } from '../utils/ble';

export default function DashboardScreen({ ble, componentId }) {
  const [status, setStatus] = useState(null);

  useEffect(() => {
    const interval = setInterval(async () => {
      if (ble.isConnected()) {
        try {
          const s = await ble.getStatus();
          setStatus(s);
        } catch (e) { /* ignore polling errors */ }
      }
    }, 1000);
    return () => clearInterval(interval);
  }, []);

  const navTo = (name) => Navigation.push(componentId, { component: { name } });

  const modeButtons = [
    { label: 'Passive Tap', mode: 0, target: 'Capture' },
    { label: 'NVMe MITM', mode: 1, target: 'Rules' },
    { label: 'Opal Interrogation', mode: 2, target: 'Opal' },
    { label: 'Controller Spoof', mode: 3, target: 'Spoof' },
    { label: 'DMA Bridge', mode: 4, target: 'Dma' },
    { label: 'FW Extract', mode: 5, target: 'Capture' },
    { label: 'Hot-Plug Fault', mode: 6, target: 'Dma' },
    { label: 'Safe Mode', mode: 8, target: null },
  ];

  const handleMode = async (mode, target) => {
    try {
      await ble.setMode(mode);
      if (target) navTo(target);
    } catch (e) { alert('Error: ' + e.message); }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Dashboard</Text>

      <View style={styles.statusCard}>
        <Text style={styles.statusRow}>Mode: <Text style={styles.val}>{status?.modeName || '---'}</Text></Text>
        <Text style={styles.statusRow}>Link: <Text style={styles.val}>Gen{status?.link || 0} x{status?.linkWidth || 0}</Text></Text>
        <Text style={styles.statusRow}>Captured: <Text style={styles.val}>{status?.captureCount || 0}</Text></Text>
        <Text style={styles.statusRow}>SD Free: <Text style={styles.val}>{status?.sdFreeKB || 0} KB</Text></Text>
        <Text style={styles.statusRow}>Battery: <Text style={styles.val}>{status?.batteryMV || 0} mV {status?.charging ? '⚡' : ''}</Text></Text>
      </View>

      <Text style={styles.sectionTitle}>Select Mode</Text>
      <View style={styles.buttonGrid}>
        {modeButtons.map((btn, i) => (
          <TouchableOpacity key={i} style={styles.modeButton} onPress={() => handleMode(btn.mode, btn.target)}>
            <Text style={styles.modeButtonText}>{btn.label}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.sectionTitle}>Tools</Text>
      <View style={styles.buttonGrid}>
        <TouchableOpacity style={styles.toolButton} onPress={() => navTo('CaptureViewer')}>
          <Text style={styles.modeButtonText}>Capture Viewer</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.toolButton} onPress={() => navTo('Settings')}>
          <Text style={styles.modeButtonText}>Settings</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.footer}>NVMe-Phantom v1.0 — jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 15 },
  statusCard: { backgroundColor: '#16213e', borderRadius: 8, padding: 15, marginBottom: 20 },
  statusRow: { color: '#ccc', fontSize: 15, marginBottom: 5 },
  val: { color: '#00AA00', fontWeight: 'bold' },
  sectionTitle: { fontSize: 18, color: '#888', marginBottom: 10, marginTop: 10 },
  buttonGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  modeButton: { backgroundColor: '#0f3460', padding: 12, borderRadius: 6, width: '48%', marginBottom: 8, alignItems: 'center' },
  toolButton: { backgroundColor: '#16213e', padding: 12, borderRadius: 6, width: '48%', marginBottom: 8, alignItems: 'center' },
  modeButtonText: { color: '#eee', fontSize: 13, fontWeight: '600' },
  footer: { color: '#555', fontSize: 11, textAlign: 'center', marginTop: 20 },
});