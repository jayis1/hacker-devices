/*
 * tpm-phantom — app/screens/InjectScreen.js
 * LPC and SPI TPM injection interface.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import {
  CMD_INJECT_LPC, CMD_INJECT_SPI,
  cmdInjectLpc, cmdInjectSpi,
} from '../utils/protocol';

export default function InjectScreen({ ble }) {
  const [lpcAddr, setLpcAddr] = useState('004E');
  const [lpcData, setLpcData] = useState('00');
  const [spiData, setSpiData] = useState('00 00 00 00');
  const [log, setLog] = useState([]);

  const addLog = (msg) => {
    setLog(prev => [`[${new Date().toLocaleTimeString()}] ${msg}`, ...prev].slice(0, 50));
  };

  const handleLpcInject = useCallback(() => {
    const addr = parseInt(lpcAddr, 16);
    const data = parseInt(lpcData, 16);
    if (isNaN(addr) || isNaN(data)) {
      Alert.alert('Error', 'Invalid hex input');
      return;
    }
    ble.send(CMD_INJECT_LPC, cmdInjectLpc(addr, data));
    addLog(`LPC WRITE addr=0x${addr.toString(16).toUpperCase()} data=0x${data.toString(16).toUpperCase()}`);
  }, [ble, lpcAddr, lpcData]);

  const handleSpiInject = useCallback(() => {
    const bytes = spiData.trim().split(/\s+/).map(s => parseInt(s, 16));
    if (bytes.some(b => isNaN(b))) {
      Alert.alert('Error', 'Invalid hex byte sequence');
      return;
    }
    ble.send(CMD_INJECT_SPI, cmdInjectSpi(new Uint8Array(bytes)));
    addLog(`SPI INJECT ${bytes.length} bytes: ${spiData}`);
  }, [ble, spiData]);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>⚠ TPM Bus Injection</Text>
        <Text style={styles.warning}>
          Injection modes actively drive the TPM bus. Only use on systems
          you own or have explicit authorization to test. Driving LPC
          lines against a live host may cause bus contention.
        </Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>LPC IO Write Injection</Text>
        <Text style={styles.label}>IO Address (hex, 16-bit):</Text>
        <TextInput
          style={styles.input}
          value={lpcAddr}
          onChangeText={setLpcAddr}
          placeholder="004E"
          placeholderTextColor="#555"
          autoCapitalize="characters"
        />
        <Text style={styles.label}>Data Byte (hex):</Text>
        <TextInput
          style={styles.input}
          value={lpcData}
          onChangeText={setLpcData}
          placeholder="00"
          placeholderTextColor="#555"
          autoCapitalize="characters"
        />
        <TouchableOpacity style={styles.injectBtn} onPress={handleLpcInject}>
          <Text style={styles.btnText}>Inject LPC IO Write</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>SPI TPM Read Response Injection</Text>
        <Text style={styles.description}>
          Pre-loads the SPI MISO line with the specified bytes. When the
          host reads from the TPM, it receives this data instead of the
          real TPM response.
        </Text>
        <Text style={styles.label}>Data bytes (hex, space-separated):</Text>
        <TextInput
          style={[styles.input, styles.multiline]}
          value={spiData}
          onChangeText={setSpiData}
          placeholder="00 00 00 00"
          placeholderTextColor="#555"
          autoCapitalize="characters"
          multiline
        />
        <TouchableOpacity style={styles.injectBtn} onPress={handleSpiInject}>
          <Text style={styles.btnText}>Inject SPI Response</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Injection Log</Text>
        {log.length === 0 ? (
          <Text style={styles.empty}>No injections performed</Text>
        ) : (
          log.map((entry, i) => (
            <Text key={i} style={styles.logEntry}>{entry}</Text>
          ))
        )}
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d1a' },
  card: {
    backgroundColor: '#1a1a2e',
    margin: 8,
    padding: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  cardTitle: {
    color: '#ff6644',
    fontSize: 15,
    fontWeight: 'bold',
    marginBottom: 8,
    fontFamily: 'monospace',
  },
  warning: {
    color: '#ff8866',
    fontSize: 11,
    fontFamily: 'monospace',
    lineHeight: 16,
  },
  description: {
    color: '#aaa',
    fontSize: 11,
    fontFamily: 'monospace',
    marginBottom: 10,
    lineHeight: 16,
  },
  label: { color: '#888', fontSize: 12, fontFamily: 'monospace', marginTop: 6 },
  input: {
    backgroundColor: '#111120',
    color: '#00ff88',
    fontFamily: 'monospace',
    fontSize: 14,
    padding: 8,
    borderRadius: 4,
    marginTop: 4,
    borderWidth: 1,
    borderColor: '#333',
  },
  multiline: { minHeight: 60 },
  injectBtn: {
    backgroundColor: '#a33',
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 12,
  },
  btnText: { color: '#fff', fontSize: 14, fontFamily: 'monospace', fontWeight: 'bold' },
  empty: { color: '#555', fontSize: 12, fontFamily: 'monospace', fontStyle: 'italic' },
  logEntry: { color: '#aaccff', fontSize: 10, fontFamily: 'monospace', marginVertical: 2 },
});