/**
 * SettingsScreen.js — Device settings and info for Joule-Phantom.
 *
 * Author : jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Linking, Alert,
} from 'react-native';
import { MODE, OP } from '../services/BleService';

const MODES = [
  { name: 'Pass-Through', code: MODE.PASSTHROUGH, desc: 'Transparent bridge — capture only, no modification' },
  { name: 'MITM',         code: MODE.MITM,        desc: 'Apply active MITM rules to transactions' },
  { name: 'Jam',          code: MODE.JAM,         desc: 'NACK all transactions (battery DoS)' },
  { name: 'Spoof Full',   code: MODE.SPOOF_FULL,  desc: 'Present a fully fake battery to host' },
  { name: 'Standby',      code: MODE.STANDBY,     desc: 'Idle — tristate both SMBus ports' },
];

export default function SettingsScreen({ sendCommand, status }) {
  const [selectedMode, setSelectedMode] = useState(MODE.PASSTHROUGH);

  const setMode = (code) => {
    setSelectedMode(code);
    sendCommand(OP.SET_MODE, [code]);
    Alert.alert('Mode Set', `Operating mode changed to: ${MODES.find((m) => m.code === code).name}`);
  };

  const clearRules = () => {
    sendCommand(OP.RULE_CLR, []);
    Alert.alert('Rules Cleared', 'All MITM and spoof rules removed.');
  };

  const authBypass = () => {
    sendCommand(OP.AUTH_BYPASS, []);
    Alert.alert('Auth Bypass Sent', 'ManufacturerAccess spoof rule installed.');
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Operating Mode</Text>
        {MODES.map((m) => (
          <TouchableOpacity
            key={m.code}
            style={[styles.modeBtn, selectedMode === m.code && styles.modeActive]}
            onPress={() => setMode(m.code)}
          >
            <View style={{ flex: 1 }}>
              <Text style={[styles.modeName, selectedMode === m.code && styles.modeNameActive]}>
                {m.name}
              </Text>
              <Text style={styles.modeDesc}>{m.desc}</Text>
            </View>
            {selectedMode === m.code && <Text style={styles.check}>✓</Text>}
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.card}>
        <Text style={styles.title}>Quick Actions</Text>
        <TouchableOpacity style={styles.actionBtn} onPress={authBypass}>
          <Text style={styles.actionText}>Enable Auth Bypass (MfrAccess spoof)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionBtn, { borderColor: '#f00' }]} onPress={clearRules}>
          <Text style={[styles.actionText, { color: '#f00' }]}>Clear All Rules</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.title}>About</Text>
        <Text style={styles.aboutText}>Joule-Phantom v1.0</Text>
        <Text style={styles.aboutText}>Smart-Battery SMBus/PMBus MITM Implant</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>License: GPL-2.0</Text>
        <Text style={styles.aboutText}>Hardware: CERN-OHL-S v2</Text>
      </View>

      <View style={styles.disclaimerCard}>
        <Text style={styles.disclaimerTitle}>⚠ Legal & Ethical Disclaimer</Text>
        <Text style={styles.disclaimerBody}>
          This device and software are designed EXCLUSIVELY for authorized
          security research, penetration testing with explicit written
          consent, and red-team operations on systems you own or have
          explicit permission to assess. Unauthorized interception or
          manipulation of battery telemetry on devices you do not own
          may violate computer-fraud statutes (e.g. 18 U.S.C. §1030),
          wiretap laws, consumer-product-safety regulations, and
          transport regulations (UN38.3, IATA DGR). Manipulating lithium
          battery telemetry carries fire and explosion risk. The author
          (jayis1) assumes no liability for misuse or damage. Always
          obtain proper written authorization before deployment.
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  card: { backgroundColor: '#16213e', margin: 10, borderRadius: 10, padding: 15 },
  title: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  modeBtn: {
    flexDirection: 'row', alignItems: 'center',
    borderWidth: 1, borderColor: '#333', borderRadius: 8,
    padding: 12, marginVertical: 4,
  },
  modeActive: { borderColor: '#e94560', backgroundColor: '#e9456022' },
  modeName: { color: '#ccc', fontSize: 14, fontWeight: '600' },
  modeNameActive: { color: '#e94560' },
  modeDesc: { color: '#777', fontSize: 11, marginTop: 2 },
  check: { color: '#e94560', fontSize: 18, marginLeft: 8 },
  actionBtn: {
    borderWidth: 1, borderColor: '#555', borderRadius: 8,
    padding: 12, marginVertical: 4, alignItems: 'center',
  },
  actionText: { color: '#aaa', fontSize: 13 },
  aboutText: { color: '#888', fontSize: 12, paddingVertical: 2 },
  disclaimerCard: {
    margin: 10, padding: 15, backgroundColor: '#2a1010',
    borderRadius: 10, borderWidth: 1, borderColor: '#f00',
  },
  disclaimerTitle: { color: '#f00', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  disclaimerBody: { color: '#ccc', fontSize: 11, lineHeight: 16 },
});