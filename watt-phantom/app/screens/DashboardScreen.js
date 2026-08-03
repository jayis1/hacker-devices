/**
 * DashboardScreen.js — real-time status overview
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Shows the current state of both USB-C ports (source and sink),
 * including PD contract, VBUS voltage/current, CC routing mode,
 * battery level, and attack mode status. Provides navigation to
 * all sub-screens.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function DashboardScreen({ navigation }) {
  const { sendCommand, status, disconnect, attackMode, setAttackMode } = useDevice();
  const [info, setInfo] = useState('');
  const [srcContract, setSrcContract] = useState('—');
  const [snkContract, setSnkContract] = useState('—');
  const [vbusSrc, setVbusSrc] = useState('—');
  const [vbusSnk, setVbusSnk] = useState('—');
  const [ccMode, setCcMode] = useState('PASSTHROUGH');
  const [batt, setBatt] = useState(100);

  const refresh = useCallback(async () => {
    const resp = await sendCommand('STATUS', 3000);
    if (resp && resp.startsWith('OK')) {
      setInfo(resp);
      // Parse the status response
      // Format: OK src=5V/3A snk=9V/2A cc=0 vbus_src=5.02V/0.31A vbus_snk=9.01V/0.52A batt=87 ...
      const srcMatch = resp.match(/src=(\w+)\/(\d+)mV\/(\d+)mA/);
      const snkMatch = resp.match(/snk=(\w+)\/(\d+)mV\/(\d+)mA/);
      const vbusSrcMatch = resp.match(/vbus_src=([\d.]+)V\/([\d.]+)A/);
      const vbusSnkMatch = resp.match(/vbus_snk=([\d.]+)V\/([\d.]+)A/);
      const ccMatch = resp.match(/cc=(\d)/);
      const battMatch = resp.match(/batt=(\d+)%/);

      if (srcMatch) setSrcContract(`${srcMatch[1]} ${srcMatch[2]}mV/${srcMatch[3]}mA`);
      if (snkMatch) setSnkContract(`${snkMatch[1]} ${snkMatch[2]}mV/${snkMatch[3]}mA`);
      if (vbusSrcMatch) setVbusSrc(`${vbusSrcMatch[1]}V / ${vbusSrcMatch[2]}A`);
      if (vbusSnkMatch) setVbusSnk(`${vbusSnkMatch[1]}V / ${vbusSnkMatch[2]}A`);
      if (ccMatch) {
        const modes = ['PASSTHROUGH', 'ISOLATED', 'CROSSOVER', 'MCU_CTRL'];
        setCcMode(modes[parseInt(ccMatch[1])] || 'UNKNOWN');
      }
      if (battMatch) setBatt(parseInt(battMatch[1]));
    }
  }, [sendCommand]);

  useEffect(() => {
    const interval = setInterval(refresh, 2000);
    refresh();
    return () => clearInterval(interval);
  }, [refresh]);

  const toggleAttackMode = async () => {
    if (!attackMode) {
      Alert.alert(
        'Enable Attack Mode?',
        'Attack mode bypasses safety limits (OVP/OCP). This can damage connected devices. Enable only on authorized targets.',
        [
          { text: 'Cancel', style: 'cancel' },
          {
            text: 'Enable',
            style: 'destructive',
            onPress: async () => {
              const resp = await sendCommand('ATTACK_MODE on', 2000);
              if (resp && resp.startsWith('OK')) setAttackMode(true);
            },
          },
        ]
      );
    } else {
      const resp = await sendCommand('ATTACK_MODE off', 2000);
      if (resp && resp.startsWith('OK')) setAttackMode(false);
    }
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>WattPhantom Dashboard</Text>
        <Text style={styles.status}>{status}</Text>
      </View>

      {/* Status cards */}
      <View style={styles.cardRow}>
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Source (Charger)</Text>
          <Text style={styles.cardValue}>{srcContract}</Text>
          <Text style={styles.cardSub}>VBUS: {vbusSrc}</Text>
        </View>
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Sink (Target)</Text>
          <Text style={styles.cardValue}>{snkContract}</Text>
          <Text style={styles.cardSub}>VBUS: {vbusSnk}</Text>
        </View>
      </View>

      <View style={styles.cardRow}>
        <View style={styles.card}>
          <Text style={styles.cardTitle}>CC Routing</Text>
          <Text style={styles.cardValue}>{ccMode}</Text>
        </View>
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Battery</Text>
          <Text style={styles.cardValue}>{batt}%</Text>
        </View>
      </View>

      {/* Attack mode warning */}
      <View style={[styles.attackCard, attackMode && styles.attackCardActive]}>
        <Text style={styles.attackText}>
          {attackMode ? '⚠️ ATTACK MODE ACTIVE' : 'Attack Mode: OFF'}
        </Text>
        <TouchableOpacity
          style={[styles.attackButton, attackMode ? styles.attackButtonOff : styles.attackButtonOn]}
          onPress={toggleAttackMode}
        >
          <Text style={styles.attackButtonText}>
            {attackMode ? 'Disable' : 'Enable'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Navigation buttons */}
      <View style={styles.navGrid}>
        <TouchableOpacity style={styles.navButton} onPress={() => navigation.navigate('PDManipulator')}>
          <Text style={styles.navButtonText}>PD Manipulator</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navButton} onPress={() => navigation.navigate('CovertChannel')}>
          <Text style={styles.navButtonText}>Covert Channel</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navButton} onPress={() => navigation.navigate('Fingerprint')}>
          <Text style={styles.navButtonText}>Fingerprint</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navButton} onPress={() => navigation.navigate('PowerMonitor')}>
          <Text style={styles.navButtonText}>Power Monitor</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navButton} onPress={() => navigation.navigate('Settings')}>
          <Text style={styles.navButtonText}>Settings</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.navButtonDisconnect} onPress={disconnect}>
          <Text style={styles.navButtonText}>Disconnect</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.author}>WattPhantom v1.0 — by jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 16 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 16 },
  title: { fontSize: 20, fontWeight: 'bold', color: '#00d4aa' },
  status: { color: '#8b949e', fontSize: 12 },
  cardRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  card: { flex: 1, backgroundColor: '#161b22', padding: 12, borderRadius: 8, borderWidth: 1, borderColor: '#30363d' },
  cardTitle: { color: '#8b949e', fontSize: 11, marginBottom: 4 },
  cardValue: { color: '#e6edf3', fontSize: 14, fontWeight: 'bold', marginBottom: 2 },
  cardSub: { color: '#6e7681', fontSize: 11 },
  attackCard: { backgroundColor: '#161b22', padding: 12, borderRadius: 8, marginBottom: 8, borderWidth: 1, borderColor: '#30363d', flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  attackCardActive: { borderColor: '#f85149', backgroundColor: '#2d1517' },
  attackText: { color: '#e6edf3', fontSize: 14, fontWeight: 'bold' },
  attackButton: { padding: 8, borderRadius: 6 },
  attackButtonOn: { backgroundColor: '#f85149' },
  attackButtonOff: { backgroundColor: '#238636' },
  attackButtonText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  navGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8, marginTop: 8 },
  navButton: { backgroundColor: '#21262d', padding: 14, borderRadius: 8, width: '48%', alignItems: 'center', marginBottom: 8 },
  navButtonDisconnect: { backgroundColor: '#2d1517', padding: 14, borderRadius: 8, width: '48%', alignItems: 'center', marginBottom: 8, borderColor: '#f85149', borderWidth: 1 },
  navButtonText: { color: '#e6edf3', fontWeight: 'bold', fontSize: 14 },
  author: { color: '#6e7681', fontSize: 11, textAlign: 'center', marginTop: 12 },
});