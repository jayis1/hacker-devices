/**
 * hart-bleeder — SettingsScreen.js
 * Device settings: BLE authentication (PSK), inter-frame timing,
 * master role, firmware info, and factory reset.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity,
  SafeAreaView, StatusBar, Alert, ScrollView, Switch,
} from 'react-native';

export default function SettingsScreen({ bleManager, connected, deviceState }) {
  const [psk, setPsk] = useState('');
  const [interframe, setInterframe] = useState('30');
  const [primaryMaster, setPrimaryMaster] = useState(true);
  const [autoReconnect, setAutoReconnect] = useState(false);

  const handleAuth = async () => {
    if (!connected) { Alert.alert('Not connected'); return; }
    if (psk.length < 8) { Alert.alert('PSK too short', 'Enter at least 8 characters.'); return; }
    try {
      await bleManager.authenticate(psk);
      Alert.alert('Authentication', 'Challenge sent. Device will validate response.');
    } catch (e) { Alert.alert('Auth Error', e.message); }
  };

  const handleReset = () => {
    Alert.alert('Factory Reset', 'Clear all stored profiles and logs?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Reset', style: 'destructive', onPress: () => {
        Alert.alert('Reset', 'Command queued (firmware reset).');
      }},
    ]);
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" />
      <ScrollView>
        <Text style={styles.title}>Settings</Text>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>BLE Authentication (XTEA-128 PSK)</Text>
          <TextInput
            style={styles.input}
            value={psk}
            onChangeText={setPsk}
            placeholder="pre-shared key (min 8 chars)"
            placeholderTextColor="#555"
            secureTextEntry
          />
          <TouchableOpacity style={styles.btn} onPress={handleAuth}>
            <Text style={styles.btnText}>Authenticate</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>HART Stack</Text>
          <View style={styles.settingRow}>
            <Text style={styles.settingLabel}>Inter-frame delay (ms)</Text>
            <TextInput
              style={[styles.input, { width: 80 }]}
              value={interframe}
              onChangeText={setInterframe}
              keyboardType="numeric"
            />
          </View>
          <View style={styles.settingRow}>
            <Text style={styles.settingLabel}>Primary Master</Text>
            <Switch
              value={primaryMaster}
              onValueChange={setPrimaryMaster}
              trackColor={{ false: '#333', true: '#4a90d9' }}
            />
          </View>
          <Text style={styles.hint}>
            HART supports two masters (primary / secondary). Set to match the
            existing control system role to avoid bus collisions.
          </Text>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Connection</Text>
          <View style={styles.settingRow}>
            <Text style={styles.settingLabel}>Auto-reconnect</Text>
            <Switch
              value={autoReconnect}
              onValueChange={setAutoReconnect}
              trackColor={{ false: '#333', true: '#10b981' }}
            />
          </View>
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Device Info</Text>
          <InfoRow label="Device" value="HART-Bleeder rev1.0" />
          <InfoRow label="Author" value="jayis1" />
          <InfoRow label="Firmware" value="0.1.0-alpha" />
          <InfoRow label="MCU" value="STM32G474CEU6" />
          <InfoRow label="Modem" value="MAX13440EETD+" />
          <InfoRow label="BLE Module" value="nRF52832-QFAA" />
          <InfoRow label="State" value={deviceState} />
        </View>

        <View style={styles.card}>
          <Text style={styles.cardTitle}>Maintenance</Text>
          <TouchableOpacity style={[styles.btn, styles.btnDanger]} onPress={handleReset}>
            <Text style={styles.btnText}>Factory Reset</Text>
          </TouchableOpacity>
        </View>

        <Text style={styles.about}>
          HART-Bleeder is a covert in-line attack device for HART (Highway
          Addressable Remote Transducer) fieldbus — the dominant digital
          overlay on 4–20 mA process control loops used across industrial
          control / SCADA. Designed for authorized red-team assessment of
          OT/ICS environments.
        </Text>
        <Text style={styles.copyright}>© 2026 jayis1 · GPL-2.0</Text>
      </ScrollView>
    </SafeAreaView>
  );
}

function InfoRow({ label, value }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 14 },
  title: { color: '#e94560', fontSize: 20, fontWeight: 'bold', marginBottom: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginBottom: 10, borderWidth: 1, borderColor: '#2d2d44' },
  cardTitle: { color: '#4a90d9', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 },
  settingLabel: { color: '#aaa', fontSize: 13 },
  input: { backgroundColor: '#0f0f1e', color: '#fff', borderRadius: 6, padding: 8, fontSize: 13, borderWidth: 1, borderColor: '#333' },
  hint: { color: '#555', fontSize: 10, marginTop: 4 },
  btn: { backgroundColor: '#2d2d44', padding: 12, borderRadius: 8, alignItems: 'center', marginTop: 6 },
  btnDanger: { backgroundColor: '#e94560' },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  infoLabel: { color: '#888', fontSize: 12 },
  infoValue: { color: '#fff', fontSize: 12, fontFamily: 'monospace' },
  about: { color: '#777', fontSize: 10, marginTop: 10, lineHeight: 16 },
  copyright: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 8, marginBottom: 20 },
});