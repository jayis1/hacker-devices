/**
 * OneWireScreen.js — 1-Wire device scanner and reader
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD } from '../utils/protocol';
import { Buffer } from 'buffer';

export default function OneWireScreen() {
  const { sendCommand } = useDevice();
  const [devices, setDevices] = useState([]);
  const [selectedDevice, setSelectedDevice] = useState(-1);
  const [readOffset, setReadOffset] = useState('00');
  const [readLen, setReadLen] = useState('32');
  const [readData, setReadData] = useState('');
  const [status, setStatus] = useState('');

  const handleScan = useCallback(async () => {
    const resp = await sendCommand(CMD.ONEWIRE_SCAN);
    if (resp && resp.length > 2) {
      const count = resp[1];
      const devs = [];
      for (let i = 0; i < count; i++) {
        const romBytes = [];
        for (let j = 0; j < 8; j++) {
          romBytes.push(resp[2 + i * 8 + j]);
        }
        const romHex = romBytes.map(b => b.toString(16).padStart(2, '0')).join(':');
        // Identify device family from first byte
        const family = romBytes[0];
        let familyName = 'Unknown';
        switch (family) {
          case 0x01: familyName = 'DS1990A (iButton)'; break;
          case 0x10: familyName = 'DS1820/DS18S20 (Temp)'; break;
          case 0x28: familyName = 'DS18B20 (Temp)'; break;
          case 0x23: familyName = 'DS2433 (EEPROM 4kb)'; break;
          case 0x2D: familyName = 'DS2431 (EEPROM 1kb)'; break;
          case 0x33: familyName = 'DS2432 (SHA-1)'; break;
          case 0x41: familyName = 'DS1923 (Temp/RH log)'; break;
        }
        devs.push({ index: i, rom: romHex, family: familyName, familyCode: family });
      }
      setDevices(devs);
      setStatus(`Found ${count} 1-Wire device(s)`);
    } else {
      setDevices([]);
      setStatus('No 1-Wire devices found');
    }
  }, [sendCommand]);

  const handleRead = useCallback(async () => {
    if (selectedDevice < 0) {
      setStatus('Select a device first');
      return;
    }
    const offset = parseInt(readOffset, 16) & 0xFF;
    const len = parseInt(readLen, 10) || 32;
    const payload = Buffer.from([selectedDevice, offset, len]);
    const resp = await sendCommand(CMD.ONEWIRE_READ, payload);
    if (resp && resp.length > 2 && resp[1] === 0) {
      const data = [];
      for (let i = 2; i < resp.length; i++) {
        data.push(resp[i].toString(16).padStart(2, '0'));
      }
      // Format as hex dump
      const lines = [];
      for (let i = 0; i < data.length; i += 16) {
        const off = (i + parseInt(readOffset, 16)).toString(16).padStart(4, '0');
        lines.push(`${off}: ${data.slice(i, i + 16).join(' ')}`);
      }
      setReadData(lines.join('\n'));
      setStatus(`Read ${data.length} bytes from device ${selectedDevice}`);
    } else {
      setStatus('1-Wire read failed');
      setReadData('');
    }
  }, [selectedDevice, readOffset, readLen, sendCommand]);

  const renderDevice = ({ item }) => (
    <TouchableOpacity
      style={[styles.deviceCard, selectedDevice === item.index && styles.deviceSelected]}
      onPress={() => setSelectedDevice(item.index)}
    >
      <View style={styles.deviceHeader}>
        <Text style={styles.deviceIndex}>Device #{item.index}</Text>
        <Text style={styles.deviceFamily}>{item.family}</Text>
      </View>
      <Text style={styles.deviceRom}>ROM: {item.rom}</Text>
      <Text style={styles.deviceFamilyCode}>Family: 0x{item.familyCode.toString(16).padStart(2, '0')}</Text>
    </TouchableOpacity>
  );

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>1-Wire Device Scan</Text>
      <TouchableOpacity style={styles.button} onPress={handleScan}>
        <Text style={styles.buttonText}>Scan 1-Wire Bus</Text>
      </TouchableOpacity>

      <FlatList
        data={devices}
        keyExtractor={item => item.index.toString()}
        renderItem={renderDevice}
        scrollEnabled={false}
        style={styles.deviceList}
        ListEmptyComponent={<Text style={styles.muted}>No devices scanned yet</Text>}
      />

      {selectedDevice >= 0 && (
        <>
          <Text style={styles.sectionTitle}>Read from Device #{selectedDevice}</Text>
          <View style={styles.row}>
            <Text style={styles.label}>Offset (hex)</Text>
            <TextInput style={styles.input} value={readOffset} onChangeText={setReadOffset} placeholder="00" />
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Length</Text>
            <TextInput style={styles.input} value={readLen} onChangeText={setReadLen} keyboardType="numeric" placeholder="32" />
          </View>
          <TouchableOpacity style={styles.button} onPress={handleRead}>
            <Text style={styles.buttonText}>Read Memory</Text>
          </TouchableOpacity>
        </>
      )}

      {readData ? (
        <View style={styles.dataBox}>
          <Text style={styles.dataText}>{readData}</Text>
        </View>
      ) : null}

      {status ? <Text style={styles.status}>{status}</Text> : null}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  button: { backgroundColor: '#e94560', padding: 12, borderRadius: 8, alignItems: 'center', marginBottom: 12 },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  deviceList: { marginBottom: 8 },
  deviceCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
    borderWidth: 2,
    borderColor: 'transparent',
  },
  deviceSelected: { borderColor: '#e94560' },
  deviceHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 4 },
  deviceIndex: { color: '#e94560', fontWeight: 'bold', fontSize: 14 },
  deviceFamily: { color: '#4ecca3', fontSize: 13 },
  deviceRom: { color: '#fff', fontFamily: 'monospace', fontSize: 12, marginBottom: 2 },
  deviceFamilyCode: { color: '#666', fontSize: 11 },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 8, gap: 8 },
  label: { color: '#888', fontSize: 13, width: 100 },
  input: { backgroundColor: '#1a1a2e', color: '#fff', padding: 8, borderRadius: 6, flex: 1, fontFamily: 'monospace' },
  dataBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginTop: 8 },
  dataText: { color: '#4ecca3', fontFamily: 'monospace', fontSize: 11 },
  muted: { color: '#666', textAlign: 'center', padding: 12 },
  status: { color: '#aaa', fontSize: 13, marginTop: 12, textAlign: 'center' },
});