/**
 * DashboardScreen.js — live phantom control & quick presets
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';

const PRESETS = [
  { name: 'Static vehicle',    range: 3000, vel: 0,       rcs: 100 },
  { name: 'Closing sweep',     range: 8000, vel: -138888, rcs: 100 },
  { name: 'RGPO walk-off',     range: 8000, vel: 0,       rcs: 80  },
  { name: 'Pedestrian (MD)',   range: 1500, vel: 600,     rcs: -48 },
  { name: '4-target cluster',  range: 8000, vel: 0,       rcs: 120 },
];

export default function DashboardScreen({ navigation }) {
  const { battery, firmware, armed, rangeCm, velMmps, rcsQdb, taps, commands } = useDevice();
  const [busy, setBusy] = useState(false);

  const rangeM = (rangeCm / 100).toFixed(1);
  const velKmh = (velMmps / 278).toFixed(0);
  const rcsDb = (rcsQdb / 8).toFixed(1);

  const applyPreset = async (p) => {
    setBusy(true);
    try {
      await commands.setRange(p.range);
      await commands.setVelocity(p.vel);
      await commands.setRcs(p.rcs);
    } finally { setBusy(false); }
  };

  const toggleArm = async () => {
    if (armed) await commands.disarm();
    else      await commands.arm();
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.statusBar}>
        <Text style={styles.statusText}>BAT {battery}%   FW {(firmware>>8)}.{firmware&0xFF}</Text>
        <Text style={[styles.statusText, { color: armed ? '#ff5555' : '#00ff9f' }]}>
          {armed ? '● ARMED' : '○ DISARMED'}
        </Text>
      </View>

      <View style={styles.paramBox}>
        <Text style={styles.paramLabel}>RANGE</Text>
        <Text style={styles.paramValue}>{rangeM} m</Text>
        <Text style={styles.paramLabel}>VELOCITY</Text>
        <Text style={styles.paramValue}>{velKmh} km/h</Text>
        <Text style={styles.paramLabel}>RCS</Text>
        <Text style={styles.paramValue}>{rcsDb} dBsm</Text>
        <Text style={styles.paramLabel}>TAPS</Text>
        <Text style={styles.paramValue}>{taps}</Text>
      </View>

      <TouchableOpacity
        style={[styles.armBtn, { backgroundColor: armed ? '#ff3333' : '#2a2a4e' }]}
        onPress={toggleArm} disabled={busy}>
        <Text style={styles.armBtnText}>{armed ? 'DISARM RF' : 'ARM RF (2-stage)'}</Text>
      </TouchableOpacity>

      <Text style={styles.section}>QUICK PRESETS</Text>
      {PRESETS.map((p, i) => (
        <TouchableOpacity key={i} style={styles.presetBtn} onPress={() => applyPreset(p)}>
          <Text style={styles.presetText}>{p.name}</Text>
          <Text style={styles.presetSub}>
            {(p.range/100).toFixed(0)} m · {(p.vel/278).toFixed(0)} km/h · {(p.rcs/8).toFixed(0)} dBsm
          </Text>
        </TouchableOpacity>
      ))}

      <Text style={styles.section}>TOOLS</Text>
      <TouchableOpacity style={styles.toolBtn} onPress={() => navigation.navigate('ScenarioEditor')}>
        <Text style={styles.toolText}>Scenario Editor</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.toolBtn} onPress={() => navigation.navigate('Target')}>
        <Text style={styles.toolText}>Multi-Target Cluster</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.toolBtn} onPress={() => navigation.navigate('BandSniff')}>
        <Text style={styles.toolText}>Band Sniff</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.toolBtn} onPress={() => navigation.navigate('Log')}>
        <Text style={styles.toolText}>Capture Log</Text>
      </TouchableOpacity>
      <TouchableOpacity style={styles.toolBtn} onPress={() => navigation.navigate('Settings')}>
        <Text style={styles.toolText}>Settings</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 16 },
  statusBar: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  statusText: { color: '#00ff9f', fontSize: 13, fontWeight: 'bold' },
  paramBox: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 16, marginBottom: 16 },
  paramLabel: { color: '#666', fontSize: 11, marginTop: 8 },
  paramValue: { color: '#fff', fontSize: 22, fontWeight: 'bold' },
  armBtn: { padding: 16, borderRadius: 10, alignItems: 'center', marginBottom: 16 },
  armBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  section: { color: '#888', fontSize: 13, marginTop: 16, marginBottom: 8, fontWeight: 'bold' },
  presetBtn: { backgroundColor: '#1a1a2e', padding: 14, borderRadius: 8, marginBottom: 6 },
  presetText: { color: '#fff', fontSize: 15, fontWeight: 'bold' },
  presetSub: { color: '#00ff9f', fontSize: 11, marginTop: 4 },
  toolBtn: { backgroundColor: '#1a1a2e', padding: 14, borderRadius: 8, marginBottom: 6 },
  toolText: { color: '#ccc', fontSize: 14 },
});