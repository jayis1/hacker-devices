/**
 * screens/SpoofScreen.js — Identify Controller spoof editor
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Picker, Alert } from 'react-native';

const DRIVE_PRESETS = [
  { name: 'Samsung PM9A3', vid: '0x144D', did: '0xA809', serial: 'S6S2NY0X123456', fw: 'GDC7602Q', model: 'SAMSUNG MZQL23T8HCLS-00A07' },
  { name: 'Samsung 980 PRO', vid: '0x144D', did: '0x9801', serial: 'S5GYNX0W654321', fw: '5B2QGXA7', model: 'Samsung SSD 980 PRO 2TB' },
  { name: 'WD SN850X', vid: '0x15B7', did: '0x7110', serial: '2140YX123456', fw: '620530WD', model: 'WD Black SN850X 2TB' },
  { name: 'Crucial T700', vid: '0x1344', did: '0x6304', serial: '2435CT123456', fw: 'CRP2-M01', model: 'CT2000T700SSD3' },
  { name: 'Intel P4510', vid: '0x8086', did: '0x0A55', serial: 'BTYL12345678', fw: 'XCV1DL2Q', model: 'INTEL SSDPE2KX020T800' },
  { name: 'Custom', vid: '0x0000', did: '0x0000', serial: '', fw: '', model: '' },
];

export default function SpoofScreen({ ble }) {
  const [preset, setPreset] = useState(DRIVE_PRESETS[0]);
  const [vid, setVid] = useState(DRIVE_PRESETS[0].vid);
  const [did, setDid] = useState(DRIVE_PRESETS[0].did);
  const [serial, setSerial] = useState(DRIVE_PRESETS[0].serial);
  const [fw, setFw] = useState(DRIVE_PRESETS[0].fw);
  const [model, setModel] = useState(DRIVE_PRESETS[0].model);
  const [capacityGB, setCapacityGB] = useState('2000');

  const handlePreset = (name) => {
    const p = DRIVE_PRESETS.find(p => p.name === name);
    setPreset(p); setVid(p.vid); setDid(p.did);
    setSerial(p.serial); setFw(p.fw); setModel(p.model);
  };

  const buildIdent = () => {
    // Build a 4096-byte Identify Controller response with spoofed fields.
    // Real NVMe Identify layout (selected offsets):
    //   0x000-0x003: VID (2B), DID (2B)
    //   0x004-0x023: Serial Number (20 bytes, ASCII + spaces)
    //   0x024-0x063: Model Number (40 bytes, ASCII + spaces)
    //   0x064-0x073: Firmware Revision (8 bytes, ASCII)
    //   0x114-0x117: Total NVM Capacity (lower 4B of 8B LE)
    //   0x118-0x11B: Total NVM Capacity (upper 4B)
    const ident = new Uint8Array(4096);
    // VID/DID
    const v = parseInt(vid, 16) & 0xFFFF;
    const d = parseInt(did, 16) & 0xFFFF;
    ident[0] = v & 0xFF; ident[1] = (v >> 8) & 0xFF;
    ident[2] = d & 0xFF; ident[3] = (d >> 8) & 0xFF;
    // Serial (offset 4, 20 bytes, space-padded ASCII)
    writeString(ident, 4, serial, 20);
    // Model (offset 36, 40 bytes)
    writeString(ident, 36, model, 40);
    // Firmware rev (offset 64, 8 bytes)
    writeString(ident, 64, fw, 8);
    // Total NVM Capacity (offset 276, 8 bytes LE) = capacityGB * 1e9
    const cap = BigInt(capacityGB) * 1000000000n;
    for (let i = 0; i < 8; i++) ident[276 + i] = Number((cap >> BigInt(i * 8)) & 0xFFn);
    return ident;
  };

  const writeString = (buf, offset, str, len) => {
    for (let i = 0; i < len; i++) {
      buf[offset + i] = i < str.length ? str.charCodeAt(i) : 0x20; // space-pad
    }
  };

  const handleApply = async () => {
    try {
      const ident = buildIdent();
      await ble.spoofIdent(ident);
      Alert.alert('Spoof Active', `Identify Controller response now returns:\n${model}\nS/N: ${serial}\nFW: ${fw}`);
    } catch (e) { Alert.alert('Error', e.message); }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Controller Spoof</Text>
      <Text style={styles.warning}>⚠️ Spoofed Identify responses can bypass drive allow-listing.{'\n'}Use only on systems you are authorized to test.</Text>

      <Text style={styles.label}>Drive Preset</Text>
      <Picker selectedValue={preset.name} style={styles.picker} onValueChange={handlePreset}>
        {DRIVE_PRESETS.map(p => <Picker.Item key={p.name} label={p.name} value={p.name} />)}
      </Picker>

      <Text style={styles.label}>Vendor ID (hex)</Text>
      <TextInput style={styles.input} value={vid} onChangeText={setVid} placeholder="0x144D"/>

      <Text style={styles.label}>Device ID (hex)</Text>
      <TextInput style={styles.input} value={did} onChangeText={setDid} placeholder="0xA809"/>

      <Text style={styles.label}>Serial Number</Text>
      <TextInput style={styles.input} value={serial} onChangeText={setSerial} placeholder="S6S2NY0X123456"/>

      <Text style={styles.label}>Firmware Revision</Text>
      <TextInput style={styles.input} value={fw} onChangeText={setFw} placeholder="GDC7602Q"/>

      <Text style={styles.label}>Model Number</Text>
      <TextInput style={styles.input} value={model} onChangeText={setModel} placeholder="Samsung SSD 980 PRO 2TB"/>

      <Text style={styles.label}>Capacity (GB)</Text>
      <TextInput style={styles.input} value={capacityGB} onChangeText={setCapacityGB} keyboardType="numeric"/>

      <TouchableOpacity style={styles.button} onPress={handleApply}>
        <Text style={styles.buttonText}>Apply Spoof to Device</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>Identify Controller = 4096B Admin response — jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 15 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#00AA00', marginBottom: 5 },
  warning: { color: '#FF6600', fontSize: 11, marginBottom: 15 },
  label: { color: '#aaa', fontSize: 14, marginTop: 10, marginBottom: 5 },
  picker: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, marginBottom: 5 },
  input: { backgroundColor: '#16213e', color: '#eee', borderRadius: 6, padding: 10, fontSize: 14, marginBottom: 5, fontFamily: 'monospace' },
  button: { backgroundColor: '#00AA00', padding: 15, borderRadius: 8, alignItems: 'center', marginTop: 15 },
  buttonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  footer: { color: '#555', fontSize: 10, textAlign: 'center', marginTop: 15 },
});