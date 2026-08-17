/**
 * InjectScreen.js — compose and send forged frames on unpowered buses
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Lets the operator compose a Modbus RTU frame (or a raw byte sequence)
 * and inject it onto an unpowered bus through the clamp. Safety
 * interlocks require jaw-closed + cable-verified-unpowered (the TDR
 * engine sets the safety flag in firmware).
 */

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, TextInput, StyleSheet, Alert } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function InjectScreen() {
  const { enterMode, injectFrame, MODE } = useDevice();
  const [slaveAddr, setSlaveAddr] = useState('1');
  const [funcCode, setFuncCode] = useState('3');
  const [startReg, setStartReg] = useState('0');
  const [quantity, setQuantity] = useState('10');
  const [writeData, setWriteData] = useState('');
  const [status, setStatus] = useState('');

  /**
   * Compute the Modbus CRC-16 (polynomial 0xA001).
   * Author: jayis1
   */
  const modbusCrc16 = (data) => {
    let crc = 0xFFFF;
    for (const b of data) {
      crc ^= b;
      for (let i = 0; i < 8; i++) {
        if (crc & 1) {
          crc = (crc >> 1) ^ 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    return crc;
  };

  const buildModbusFrame = () => {
    const addr = parseInt(slaveAddr, 10) || 1;
    const func = parseInt(funcCode, 10) || 3;
    const start = parseInt(startReg, 10) || 0;
    const qty = parseInt(quantity, 10) || 1;

    const pdu = [addr, func];
    // Read-type frames: start addr (2B) + quantity (2B)
    if ([1, 2, 3, 4].includes(func)) {
      pdu.push((start >> 8) & 0xFF, start & 0xFF);
      pdu.push((qty >> 8) & 0xFF, qty & 0xFF);
    } else if ([5, 6].includes(func)) {
      // Write single: addr (2B) + value (2B)
      pdu.push((start >> 8) & 0xFF, start & 0xFF);
      pdu.push((qty >> 8) & 0xFF, qty & 0xFF);
    } else if ([15, 16].includes(func)) {
      // Write multiple: start (2B) + qty (2B) + bytecount (1B) + data
      pdu.push((start >> 8) & 0xFF, start & 0xFF);
      pdu.push((qty >> 8) & 0xFF, qty & 0xFF);
      const dataBytes = writeData.trim().split(/[\s,]+/).map((h) => parseInt(h, 16));
      pdu.push(dataBytes.length & 0xFF);
      pdu.push(...dataBytes);
    }
    const crc = modbusCrc16(pdu);
    pdu.push(crc & 0xFF, (crc >> 8) & 0xFF);
    return pdu;
  };

  const handleInject = async () => {
    Alert.alert(
      'Confirm Injection',
      'Ensure the bus is UNPOWERED and you have authorisation to inject. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Inject',
          style: 'destructive',
          onPress: async () => {
            await enterMode(MODE.INJECT);
            const frame = buildModbusFrame();
            const resp = await injectFrame(Buffer.from(frame));
            setStatus(`Sent ${frame.length} bytes. Response: ${resp}`);
          },
        },
      ]
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Inject Frame</Text>
      <Text style={styles.warning}>
        ⚠️ Inject mode is for UNPOWERED buses only. The firmware refuses to
        inject if the TDR engine detects a live conductor. Always verify
        authorisation scope before injecting.
      </Text>

      <View style={styles.form}>
        <Text style={styles.sectionTitle}>Modbus RTU Frame Builder</Text>

        <View style={styles.row}>
          <View style={styles.field}>
            <Text style={styles.label}>Slave addr</Text>
            <TextInput style={styles.input} value={slaveAddr} onChangeText={setSlaveAddr} keyboardType="numeric" />
          </View>
          <View style={styles.field}>
            <Text style={styles.label}>Function</Text>
            <TextInput style={styles.input} value={funcCode} onChangeText={setFuncCode} keyboardType="numeric" />
          </View>
        </View>

        <View style={styles.row}>
          <View style={styles.field}>
            <Text style={styles.label}>Start register</Text>
            <TextInput style={styles.input} value={startReg} onChangeText={setStartReg} keyboardType="numeric" />
          </View>
          <View style={styles.field}>
            <Text style={styles.label}>Quantity / value</Text>
            <TextInput style={styles.input} value={quantity} onChangeText={setQuantity} keyboardType="numeric" />
          </View>
        </View>

        {(parseInt(funcCode, 10) === 15 || parseInt(funcCode, 10) === 16) && (
          <View style={styles.field}>
            <Text style={styles.label}>Write data (hex, space-separated)</Text>
            <TextInput
              style={[styles.input, { height: 60 }]}
              value={writeData}
              onChangeText={setWriteData}
              placeholder="00 0A FF"
              placeholderTextColor="#6e7681"
            />
          </View>
        )}

        <Text style={styles.preview}>
          Frame: [{buildModbusFrame().map((b) => b.toString(16).padStart(2, '0')).join(' ')}]
        </Text>

        <TouchableOpacity style={styles.injectButton} onPress={handleInject}>
          <Text style={styles.injectButtonText}>⚡ Inject Frame</Text>
        </TouchableOpacity>

        {status ? <Text style={styles.status}>{status}</Text> : null}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0d1117' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#f85149', marginBottom: 8 },
  warning: { color: '#f85149', fontSize: 11, marginBottom: 16, backgroundColor: '#161b22', padding: 10, borderRadius: 6 },
  form: { backgroundColor: '#161b22', padding: 16, borderRadius: 10, borderWidth: 1, borderColor: '#30363d' },
  sectionTitle: { color: '#f0f6fc', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  row: { flexDirection: 'row', gap: 12, marginBottom: 10 },
  field: { flex: 1 },
  label: { color: '#8b949e', fontSize: 12, marginBottom: 4 },
  input: { backgroundColor: '#0d1117', color: '#f0f6fc', borderRadius: 6, paddingHorizontal: 10, height: 38, borderWidth: 1, borderColor: '#30363d' },
  preview: { color: '#00d4aa', fontSize: 11, fontFamily: 'monospace', marginTop: 10, marginBottom: 10 },
  injectButton: { backgroundColor: '#f85149', padding: 14, borderRadius: 8, alignItems: 'center' },
  injectButtonText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  status: { color: '#8b949e', fontSize: 12, marginTop: 10 },
});