/**
 * I2cConsoleScreen.js — I2C bus scanner, reader, writer, emulator
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, buildI2cScan, buildI2cRead, buildI2cWrite, buildI2cEmulate } from '../utils/protocol';
import { Buffer } from 'buffer';

export default function I2cConsoleScreen() {
  const { sendCommand } = useDevice();
  const [channel, setChannel] = useState('0');
  const [scanResults, setScanResults] = useState([]);
  const [addr, setAddr] = useState('50');
  const [reg, setReg] = useState('00');
  const [readLen, setReadLen] = useState('16');
  const [readData, setReadData] = useState('');
  const [writeData, setWriteData] = useState('');
  const [emulateAddr, setEmulateAddr] = useState('50');
  const [status, setStatus] = useState('');

  const handleScan = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const resp = await sendCommand(CMD.I2C_SCAN, buildI2cScan(ch));
    if (resp && resp.length > 4) {
      const count = resp[3];
      const addrs = [];
      for (let i = 0; i < count; i++) {
        addrs.push('0x' + resp[4 + i].toString(16).padStart(2, '0'));
      }
      setScanResults(addrs);
      setStatus(`Found ${count} device(s) on I2C${ch}`);
    } else {
      setStatus('Scan failed');
    }
  }, [channel, sendCommand]);

  const handleRead = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const a = parseInt(addr, 16) & 0x7F;
    const r = parseInt(reg, 16) & 0xFF;
    const len = parseInt(readLen, 10) || 16;
    const resp = await sendCommand(CMD.I2C_READ, buildI2cRead(ch, a, r, len));
    if (resp && resp.length > 3 && resp[1] === 0) {
      const data = [];
      for (let i = 3; i < resp.length; i++) {
        data.push(resp[i].toString(16).padStart(2, '0'));
      }
      setReadData(data.join(' '));
      setStatus(`Read ${data.length} bytes from 0x${addr} reg 0x${reg}`);
    } else {
      setStatus('Read failed (NACK or timeout)');
      setReadData('');
    }
  }, [channel, addr, reg, readLen, sendCommand]);

  const handleWrite = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const a = parseInt(addr, 16) & 0x7F;
    const r = parseInt(reg, 16) & 0xFF;
    const dataBytes = writeData.trim().split(/[\s,]+/).map(s => parseInt(s, 16) & 0xFF).filter(n => !isNaN(n));
    const resp = await sendCommand(CMD.I2C_WRITE, buildI2cWrite(ch, a, r, dataBytes));
    if (resp && resp[1] === 0) {
      setStatus(`Wrote ${dataBytes.length} bytes to 0x${addr} reg 0x${reg}`);
    } else {
      setStatus('Write failed (NACK)');
    }
  }, [channel, addr, reg, writeData, sendCommand]);

  const handleEmulate = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const a = parseInt(emulateAddr, 16) & 0x7F;
    const resp = await sendCommand(CMD.I2C_EMULATE, buildI2cEmulate(ch, a));
    if (resp && resp[1] === 0) {
      setStatus(`Emulating I2C slave at 0x${emulateAddr} on ch${ch}`);
    } else {
      setStatus('Emulate failed');
    }
  }, [channel, emulateAddr, sendCommand]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>I²C Channel</Text>
      <View style={styles.row}>
        <TextInput style={styles.input} value={channel} onChangeText={setChannel} keyboardType="numeric" placeholder="0" />
        <Text style={styles.hint}>0 or 1</Text>
      </View>

      {/* Scan */}
      <Text style={styles.sectionTitle}>Bus Scan</Text>
      <TouchableOpacity style={styles.button} onPress={handleScan}>
        <Text style={styles.buttonText}>Scan I²C Bus</Text>
      </TouchableOpacity>
      <View style={styles.scanResults}>
        {scanResults.length === 0 ? (
          <Text style={styles.muted}>No devices found yet</Text>
        ) : (
          <View style={styles.addrGrid}>
            {scanResults.map(a => (
              <View key={a} style={styles.addrBadge}>
                <Text style={styles.addrText}>{a}</Text>
              </View>
            ))}
          </View>
        )}
      </View>

      {/* Read */}
      <Text style={styles.sectionTitle}>Read from Device</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Addr (hex)</Text>
        <TextInput style={styles.input} value={addr} onChangeText={setAddr} placeholder="50" />
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Reg (hex)</Text>
        <TextInput style={styles.input} value={reg} onChangeText={setReg} placeholder="00" />
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Length</Text>
        <TextInput style={styles.input} value={readLen} onChangeText={setReadLen} keyboardType="numeric" placeholder="16" />
      </View>
      <TouchableOpacity style={styles.button} onPress={handleRead}>
        <Text style={styles.buttonText}>Read</Text>
      </TouchableOpacity>
      {readData ? (
        <View style={styles.dataBox}>
          <Text style={styles.dataText}>{readData}</Text>
        </View>
      ) : null}

      {/* Write */}
      <Text style={styles.sectionTitle}>Write to Device</Text>
      <TextInput
        style={[styles.input, styles.textArea]}
        value={writeData}
        onChangeText={setWriteData}
        placeholder="AA BB CC (hex bytes)"
        multiline
      />
      <TouchableOpacity style={styles.button} onPress={handleWrite}>
        <Text style={styles.buttonText}>Write</Text>
      </TouchableOpacity>

      {/* Emulate */}
      <Text style={styles.sectionTitle}>Slave Emulation</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Emulate Addr</Text>
        <TextInput style={styles.input} value={emulateAddr} onChangeText={setEmulateAddr} placeholder="50" />
      </View>
      <TouchableOpacity style={[styles.button, styles.emulateButton]} onPress={handleEmulate}>
        <Text style={styles.buttonText}>Start Emulating</Text>
      </TouchableOpacity>

      {status ? <Text style={styles.status}>{status}</Text> : null}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  sectionTitle: { color: '#e94560', fontSize: 16, fontWeight: 'bold', marginTop: 16, marginBottom: 8 },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 8, gap: 8 },
  label: { color: '#888', fontSize: 13, width: 100 },
  input: {
    backgroundColor: '#1a1a2e',
    color: '#fff',
    padding: 8,
    borderRadius: 6,
    flex: 1,
    fontSize: 14,
    fontFamily: 'monospace',
  },
  textArea: { minHeight: 60, textAlignVertical: 'top' },
  hint: { color: '#555', fontSize: 11 },
  button: {
    backgroundColor: '#e94560',
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 8,
  },
  emulateButton: { backgroundColor: '#4ecca3' },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  scanResults: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 8 },
  addrGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  addrBadge: { backgroundColor: '#2a2a4e', borderRadius: 4, padding: 6 },
  addrText: { color: '#4ecca3', fontFamily: 'monospace', fontSize: 13 },
  muted: { color: '#666', fontSize: 13 },
  dataBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginTop: 8 },
  dataText: { color: '#4ecca3', fontFamily: 'monospace', fontSize: 13 },
  status: { color: '#aaa', fontSize: 13, marginTop: 12, textAlign: 'center' },
});