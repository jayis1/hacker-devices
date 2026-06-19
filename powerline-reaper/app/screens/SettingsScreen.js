// screens/SettingsScreen.js — firmware/region/zeroize/tamper
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, Switch } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import { MSG } from '../utils/protocol';

export default function SettingsScreen() {
  const { send, status } = useReaper();

  const zeroize = () => {
    Alert.alert('Zeroize', 'Wipe all keys, NMK, and SD. The device will halt. Continue?', [
      { text: 'Cancel' },
      { text: 'Zeroize', style: 'destructive', onPress: () => send(MSG.ZEROIZE) },
    ]);
  };

  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.header}>
        <Text style={styles.h1}>Settings</Text>
        <Text style={styles.author}>Powerline-Reaper · author: jayis1 · MIT</Text>
      </View>

      <Section title="Region / Plug Module">
        <SettingRow label="US (NEMA 5-15P)" onPress={() => send(MSG.SET_REGION, new Uint8Array([0]))} />
        <SettingRow label="EU (Schuko)"     onPress={() => send(MSG.SET_REGION, new Uint8Array([1]))} />
        <SettingRow label="UK (BS 1363)"     onPress={() => send(MSG.SET_REGION, new Uint8Array([2]))} />
        <SettingRow label="AU (AS/NZS 3112)" onPress={() => send(MSG.SET_REGION, new Uint8Array([3]))} />
      </Section>

      <Section title="Firmware">
        <SettingRow label="OTA Update (begin)" onPress={() => send(MSG.OTA_BEGIN)} />
        <SettingRow label="OTA End + reboot"    onPress={() => send(MSG.OTA_END)} />
        <Text style={styles.help}>Firmware images are pushed over BLE in 1 KB chunks via OTA_CHUNK messages.</Text>
      </Section>

      <Section title="Security">
        <SettingRow label="Zeroize Keys + SD" onPress={zeroize} danger />
        <Text style={styles.help}>On chassis-open (tamper switch), the device auto-zeroizes keys and crypto-erases the SD card. Mode is now: {status.mode}</Text>
      </Section>

      <Section title="Legal">
        <Text style={styles.legal}>
          For authorized security research and penetration testing only.
          Use only on infrastructure you own or have explicit written
          authorization to assess. Connecting non-approved transmit apparatus
          to AC mains may violate electrical codes and FCC Part 15 emission
          limits. The author (jayis1) assumes no liability for misuse.
        </Text>
      </Section>
    </View>
  );
}

const Section = ({ title, children }) => (
  <View style={styles.section}>
    <Text style={styles.h2}>{title}</Text>
    {children}
  </View>
);

const SettingRow = ({ label, onPress, danger }) => (
  <TouchableOpacity style={[styles.row, danger && styles.rowDanger]} onPress={onPress}>
    <Text style={styles.rowLabel}>{label}</Text>
    <Text style={styles.rowArrow}>›</Text>
  </TouchableOpacity>
);

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  header: { marginVertical: 12 },
  h1: { color: '#eee', fontSize: 20, fontWeight: 'bold' },
  author: { color: '#555', fontSize: 11, marginTop: 4 },
  section: { backgroundColor: '#111', padding: 12, borderRadius: 6, marginBottom: 12 },
  h2: { color: '#00ffaa', fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 10, borderBottomWidth: 0.5, borderBottomColor: '#222' },
  rowDanger: { backgroundColor: '#2a0a0a', borderRadius: 4, paddingHorizontal: 8 },
  rowLabel: { color: '#eee', fontSize: 13 },
  rowArrow: { color: '#666', fontSize: 16 },
  help: { color: '#666', fontSize: 11, marginTop: 6, lineHeight: 15 },
  legal: { color: '#888', fontSize: 10, lineHeight: 14 },
});