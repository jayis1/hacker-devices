/**
 * DashboardScreen.js — main dashboard after connection
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function DashboardScreen({ navigation }) {
  const { status, battery, currentMode, firmwareVersion, MODE } = useDevice();

  const modeNames = {
    [MODE.IDLE]: 'Idle',
    [MODE.TDR]: 'TDR',
    [MODE.SNIFF]: 'Sniff',
    [MODE.INJECT]: 'Inject',
    [MODE.COVERT]: 'Covert',
    [MODE.CABLEMAP]: 'Cable Map',
  };

  const cards = [
    { title: 'TDR Reflectogram', desc: 'Fire TDR, view cable classification', target: 'TDRReflectogram', color: '#00d4aa' },
    { title: 'Live Capture', desc: 'Sniff decoded fieldbus frames', target: 'LiveCapture', color: '#ff6b35' },
    { title: 'Cable Map', desc: 'Aggregate TDR results into a cable inventory', target: 'CableMap', color: '#a371f7' },
    { title: 'Inject', desc: 'Send forged frames on unpowered buses', target: 'Inject', color: '#f85149' },
    { title: 'Covert Channel', desc: 'Pair two devices for air-gap bridging', target: 'CovertChannel', color: '#d29922' },
    { title: 'Settings', desc: 'BLE key management, firmware update', target: 'Settings', color: '#8b949e' },
  ];

  return (
    <View style={styles.container}>
      <View style={styles.headerBar}>
        <Text style={styles.headerTitle}>Pulse-Reaper</Text>
        <View style={styles.statusBar}>
          <Text style={styles.statusText}>{modeNames[currentMode] || 'Unknown'}</Text>
          <Text style={styles.batteryText}>🔋 {battery}%</Text>
        </View>
      </View>

      <Text style={styles.fwVersion}>FW: {firmwareVersion || 'unknown'}</Text>
      <Text style={styles.statusText}>Status: {status}</Text>

      <View style={styles.grid}>
        {cards.map((card) => (
          <TouchableOpacity
            key={card.title}
            style={[styles.card, { borderColor: card.color }]}
            onPress={() => navigation.navigate(card.target)}
          >
            <Text style={[styles.cardTitle, { color: card.color }]}>{card.title}</Text>
            <Text style={styles.cardDesc}>{card.desc}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.footer}>© 2026 jayis1 — MIT License — Authorized use only</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0d1117' },
  headerBar: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 },
  headerTitle: { fontSize: 22, fontWeight: 'bold', color: '#ff6b35' },
  statusBar: { flexDirection: 'row', gap: 12 },
  statusText: { color: '#8b949e', fontSize: 12 },
  batteryText: { color: '#00d4aa', fontSize: 14 },
  fwVersion: { color: '#6e7681', fontSize: 11, marginBottom: 12 },
  grid: { flex: 1, flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  card: { width: '48%', backgroundColor: '#161b22', padding: 16, borderRadius: 10, marginBottom: 12, borderWidth: 1.5 },
  cardTitle: { fontSize: 16, fontWeight: 'bold', marginBottom: 6 },
  cardDesc: { color: '#8b949e', fontSize: 12 },
  footer: { color: '#6e7681', fontSize: 10, textAlign: 'center', marginTop: 8 },
});