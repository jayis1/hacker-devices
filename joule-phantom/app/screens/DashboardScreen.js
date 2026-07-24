/**
 * DashboardScreen.js — Live telemetry dashboard for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Shows real-time battery status as reported by the device: operating
 * mode, battery-present flag, rule count, temperature, voltage, and
 * connection tick.  Provides quick-launch buttons for spoof profiles.
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { MODE, OP } from '../services/BleService';

const SBS_PROFILES = [
  { label: 'Full Charge',  mode: MODE.SPOOF_FULL, color: '#0f0', desc: 'Spoof 100% SoC, 12.6V' },
  { label: 'Empty',        mode: MODE.MITM,       color: '#f00', desc: 'Spoof 1% SoC, 3.0V — forces shutdown' },
  { label: 'Over-Temp',    mode: MODE.MITM,       color: '#f80', desc: 'Inject 60C over-temperature alarm' },
  { label: 'Over-Current', mode: MODE.MITM,       color: '#f0f', desc: 'Inject 8A over-current alarm' },
  { label: 'No Charge',    mode: MODE.MITM,       color: '#08f', desc: 'Force charging current/voltage = 0' },
  { label: 'Auth Bypass',  mode: MODE.MITM,       color: '#ff0', desc: 'Spoof ManufacturerAccess ACK' },
  { label: 'Pass-Through', mode: MODE.PASSTHROUGH,color: '#888', desc: 'Transparent bridge, capture only' },
  { label: 'Standby',      mode: MODE.STANDBY,    color: '#444', desc: 'Idle — tristate both ports' },
];

export default function DashboardScreen({ status, connected, sendCommand }) {
  const applyProfile = (p) => {
    sendCommand(OP.SET_MODE, [p.mode]);
    if (p.label === 'Auth Bypass')  sendCommand(OP.AUTH_BYPASS, []);
    if (p.label === 'Full Charge')  sendCommand(OP.SET_MODE, [MODE.SPOOF_FULL]);
    if (p.label === 'Pass-Through') sendCommand(OP.RULE_CLR, []);
  };

  const tempK = status.temperature || 0;
  const tempC = (tempK / 10 - 273.15).toFixed(1);
  const voltRaw = status.voltage || 0;
  const voltV = (voltRaw / 1000).toFixed(3); // approx from ADC raw

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Status</Text>
        <Row label="Connection" value={connected ? 'Online' : 'Offline'} />
        <Row label="Operating Mode" value={status.mode || '—'} highlight />
        <Row label="Battery Present" value={status.batteryPresent ? 'Yes' : 'No'} />
        <Row label="Active Rules" value={String(status.ruleCount ?? 0)} />
        <Row label="Temperature" value={tempC + ' °C'} />
        <Row label="Voltage (ADC)" value={voltV + ' V'} />
        <Row label="Tick" value={String(status.tick ?? 0)} />
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Spoof Profiles</Text>
        <Text style={styles.warn}>
          ⚠ Authorized testing only. Misuse may damage equipment or
          cause fire. See legal disclaimer.
        </Text>
        {SBS_PROFILES.map((p) => (
          <TouchableOpacity
            key={p.label}
            style={[styles.profileBtn, { borderColor: p.color }]}
            onPress={() => applyProfile(p)}
          >
            <View style={{ flex: 1 }}>
              <Text style={[styles.profileLabel, { color: p.color }]}>{p.label}</Text>
              <Text style={styles.profileDesc}>{p.desc}</Text>
            </View>
            <Text style={styles.arrow}>→</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.footer}>
        <Text style={styles.footerText}>Joule-Phantom v1.0 — Author: jayis1 — GPL-2.0</Text>
      </View>
    </ScrollView>
  );
}

function Row({ label, value, highlight }) {
  return (
    <View style={styles.row}>
      <Text style={styles.rowLabel}>{label}</Text>
      <Text style={[styles.rowValue, highlight && styles.highlight]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  card: {
    backgroundColor: '#16213e',
    margin: 10,
    borderRadius: 10,
    padding: 15,
  },
  cardTitle: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  warn: { color: '#f80', fontSize: 11, marginBottom: 10, fontStyle: 'italic' },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  rowLabel: { color: '#aaa', fontSize: 13 },
  rowValue: { color: '#fff', fontSize: 13, fontWeight: '600' },
  highlight: { color: '#e94560', fontSize: 15 },
  profileBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderRadius: 8,
    padding: 12,
    marginVertical: 4,
    backgroundColor: '#1a1a2e',
  },
  profileLabel: { fontSize: 15, fontWeight: 'bold' },
  profileDesc: { color: '#888', fontSize: 11, marginTop: 2 },
  arrow: { color: '#666', fontSize: 18, marginLeft: 10 },
  footer: { padding: 20, alignItems: 'center' },
  footerText: { color: '#444', fontSize: 10 },
});