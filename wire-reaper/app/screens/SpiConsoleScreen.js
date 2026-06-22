/**
 * SpiConsoleScreen.js — SPI flash dump, read, write, and slave emulation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, buildSpiRead, buildSpiWrite } from '../utils/protocol';
import { Buffer } from 'buffer';

export default function SpiConsoleScreen() {
  const { sendCommand } = useDevice();
  const [channel, setChannel] = useState('0');
  const [cmdBytes, setCmdBytes] = useState('03'); // READ command
  const [readLen, setReadLen] = useState('256');
  const [readData, setReadData] = useState('');
  const [writeData, setWriteData] = useState('');
  const [flashAddr, setFlashAddr] = useState('000000');
  const [dumpProgress, setDumpProgress] = useState('');
  const [jedecId, setJedecId] = useState('');
  const [status, setStatus] = useState('');

  // Read using raw command bytes
  const handleRead = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const cmd = cmdBytes.trim().split(/[\s,]+/).map(s => parseInt(s, 16) & 0xFF).filter(n => !isNaN(n));
    const len = parseInt(readLen, 10) || 256;
    const resp = await sendCommand(CMD.SPI_READ, buildSpiRead(ch, cmd, len));
    if (resp && resp.length > 2 && resp[1] === 0) {
      const data = [];
      for (let i = 2; i < resp.length; i++) {
        data.push(resp[i].toString(16).padStart(2, '0'));
      }
      // Format as hex dump (16 bytes per line)
      const lines = [];
      for (let i = 0; i < data.length; i += 16) {
        const offset = i.toString(16).padStart(4, '0');
        const hex = data.slice(i, i + 16).join(' ');
        lines.push(`${offset}: ${hex}`);
      }
      setReadData(lines.join('\n'));
      setStatus(`Read ${data.length} bytes`);
    } else {
      setStatus('SPI read failed');
      setReadData('');
    }
  }, [channel, cmdBytes, readLen, sendCommand]);

  // Read JEDEC ID (cmd 0x9F)
  const handleJedec = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const resp = await sendCommand(CMD.SPI_READ, buildSpiRead(ch, [0x9F], 6));
    if (resp && resp.length > 2 && resp[1] === 0) {
      const id = [];
      for (let i = 2; i < resp.length; i++) {
        id.push(resp[i].toString(16).padStart(2, '0'));
      }
      setJedecId(id.join(' '));
      setStatus(`JEDEC ID: ${id.join(' ')}`);
    } else {
      setStatus('JEDEC ID read failed');
    }
  }, [channel, sendCommand]);

  // Dump SPI flash starting at address
  const handleFlashDump = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const addr = parseInt(flashAddr, 16) & 0xFFFFFF;
    const blockSize = 256;
    const totalBlocks = 256; // 256 * 256 = 64KB dump
    let allData = [];

    setDumpProgress(`Dumping 64KB from 0x${flashAddr}...`);
    for (let block = 0; block < totalBlocks; block++) {
      const cmd = [0x03, (addr + block * blockSize) >> 16 & 0xFF, (addr + block * blockSize) >> 8 & 0xFF, (addr + block * blockSize) & 0xFF];
      const resp = await sendCommand(CMD.SPI_READ, buildSpiRead(ch, cmd, blockSize));
      if (resp && resp[1] === 0) {
        for (let i = 2; i < resp.length; i++) {
          allData.push(resp[i]);
        }
        setDumpProgress(`Block ${block + 1}/${totalBlocks} (${((block + 1) * 100 / totalBlocks).toFixed(0)}%)`);
      } else {
        setStatus(`Flash dump failed at block ${block}`);
        return;
      }
    }

    // Format first 256 bytes as preview
    const preview = allData.slice(0, 256).map(b => b.toString(16).padStart(2, '0'));
    const lines = [];
    for (let i = 0; i < preview.length; i += 16) {
      const offset = i.toString(16).padStart(4, '0');
      lines.push(`${offset}: ${preview.slice(i, i + 16).join(' ')}`);
    }
    setReadData(lines.join('\n') + '\n... (64KB total)');
    setStatus(`Flash dump complete: ${allData.length} bytes`);
    setDumpProgress('');
  }, [channel, flashAddr, sendCommand]);

  const handleWrite = useCallback(async () => {
    const ch = parseInt(channel, 10) || 0;
    const cmd = cmdBytes.trim().split(/[\s,]+/).map(s => parseInt(s, 16) & 0xFF).filter(n => !isNaN(n));
    const data = writeData.trim().split(/[\s,]+/).map(s => parseInt(s, 16) & 0xFF).filter(n => !isNaN(n));
    const resp = await sendCommand(CMD.SPI_WRITE, buildSpiWrite(ch, cmd, data));
    if (resp && resp[1] === 0) {
      setStatus(`Wrote ${data.length} bytes`);
    } else {
      setStatus('SPI write failed');
    }
  }, [channel, cmdBytes, writeData, sendCommand]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>SPI Channel</Text>
      <View style={styles.row}>
        <TextInput style={styles.input} value={channel} onChangeText={setChannel} keyboardType="numeric" placeholder="0" />
        <Text style={styles.hint}>0 or 1</Text>
      </View>

      {/* Quick actions */}
      <Text style={styles.sectionTitle}>Quick Actions</Text>
      <View style={styles.quickRow}>
        <TouchableOpacity style={[styles.button, styles.quickButton]} onPress={handleJedec}>
          <Text style={styles.buttonText}>Read JEDEC ID</Text>
        </TouchableOpacity>
      </View>
      {jedecId ? (
        <View style={styles.dataBox}>
          <Text style={styles.dataText}>JEDEC ID: {jedecId}</Text>
        </View>
      ) : null}

      {/* Flash dump */}
      <Text style={styles.sectionTitle}>SPI Flash Dump (64KB)</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Start Addr (hex)</Text>
        <TextInput style={styles.input} value={flashAddr} onChangeText={setFlashAddr} placeholder="000000" />
      </View>
      <TouchableOpacity style={[styles.button, styles.dumpButton]} onPress={handleFlashDump}>
        <Text style={styles.buttonText}>Dump 64KB</Text>
      </TouchableOpacity>
      {dumpProgress ? <Text style={styles.progress}>{dumpProgress}</Text> : null}

      {/* Manual read */}
      <Text style={styles.sectionTitle}>Manual SPI Read</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Cmd bytes (hex)</Text>
        <TextInput style={styles.input} value={cmdBytes} onChangeText={setCmdBytes} placeholder="03" />
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Read length</Text>
        <TextInput style={styles.input} value={readLen} onChangeText={setReadLen} keyboardType="numeric" placeholder="256" />
      </View>
      <TouchableOpacity style={styles.button} onPress={handleRead}>
        <Text style={styles.buttonText}>Read SPI</Text>
      </TouchableOpacity>

      {/* Manual write */}
      <Text style={styles.sectionTitle}>Manual SPI Write</Text>
      <TextInput
        style={[styles.input, styles.textArea]}
        value={writeData}
        onChangeText={setWriteData}
        placeholder="AA BB CC (hex bytes to write)"
        multiline
      />
      <TouchableOpacity style={styles.button} onPress={handleWrite}>
        <Text style={styles.buttonText}>Write SPI</Text>
      </TouchableOpacity>

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
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 8, gap: 8 },
  label: { color: '#888', fontSize: 13, width: 120 },
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
  quickRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  button: { backgroundColor: '#e94560', padding: 12, borderRadius: 8, alignItems: 'center', marginBottom: 8 },
  quickButton: { flex: 1 },
  dumpButton: { backgroundColor: '#7ec8e3' },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  dataBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginTop: 8 },
  dataText: { color: '#4ecca3', fontFamily: 'monospace', fontSize: 12 },
  progress: { color: '#7ec8e3', fontSize: 13, textAlign: 'center', marginBottom: 8 },
  status: { color: '#aaa', fontSize: 13, marginTop: 12, textAlign: 'center' },
});