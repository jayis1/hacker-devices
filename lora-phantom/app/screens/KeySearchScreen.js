/*
 * lora-phantom / app / screens / KeySearchScreen.js
 * Offline MIC brute-force against captured join-request.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, ActivityIndicator } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, buildKeySearchDict, hexToBytes, hexStr } from '../utils/protocol';

export default function KeySearchScreen() {
  const { connected, joinReqs, keyResult, sendCommand } = useDevice();
  const [selectedJr, setSelectedJr] = useState(0);
  const [dictHex, setDictHex] = useState('00000000000000000000000000000000\nFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF');
  const [running, setRunning] = useState(false);
  const [customKey, setCustomKey] = useState('');

  useEffect(() => {
    if (keyResult) setRunning(false);
  }, [keyResult]);

  const runDictionary = () => {
    if (!joinReqs[selectedJr]) return;
    const jr = joinReqs[selectedJr];
    // Reconstruct raw join-request bytes (19 bytes)
    const jrRaw = new Uint8Array(19);
    jrRaw[0] = 0x00; // MHDR
    const euiBytes = jr.joinEui.split(':').map(h => parseInt(h, 16));
    const devBytes = jr.devEui.split(':').map(h => parseInt(h, 16));
    for (let i = 0; i < 8; i++) jrRaw[1 + i] = euiBytes[i];
    for (let i = 0; i < 8; i++) jrRaw[9 + i] = devBytes[i];
    jrRaw[17] = jr.devNonce & 0xFF;
    jrRaw[18] = (jr.devNonce >> 8) & 0xFF;

    // Parse dictionary
    const lines = dictHex.split('\n').map(s => s.trim()).filter(s => s.length === 32);
    const dict = lines.map(h => hexToBytes(h));

    const payload = buildKeySearchDict(jrRaw, dict);
    sendCommand(CMD.KEYSEARCH_RUN, payload);
    setRunning(true);
  };

  const runCustomKey = () => {
    if (customKey.length !== 32 || !joinReqs[selectedJr]) return;
    const jr = joinReqs[selectedJr];
    const jrRaw = new Uint8Array(19);
    jrRaw[0] = 0x00;
    const euiBytes = jr.joinEui.split(':').map(h => parseInt(h, 16));
    const devBytes = jr.devEui.split(':').map(h => parseInt(h, 16));
    for (let i = 0; i < 8; i++) jrRaw[1 + i] = euiBytes[i];
    for (let i = 0; i < 8; i++) jrRaw[9 + i] = devBytes[i];
    jrRaw[17] = jr.devNonce & 0xFF;
    jrRaw[18] = (jr.devNonce >> 8) & 0xFF;
    const dict = [hexToBytes(customKey)];
    const payload = buildKeySearchDict(jrRaw, dict);
    sendCommand(CMD.KEYSEARCH_RUN, payload);
    setRunning(true);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>🔑 Key Search</Text>

      <Text style={styles.section}>Select Join-Request</Text>
      {joinReqs.length === 0 ? (
        <Text style={styles.empty}>No join-requests captured. Run the sniffer first.</Text>
      ) : (
        joinReqs.map((jr, i) => (
          <TouchableOpacity key={i}
            style={[styles.jrItem, selectedJr === i && styles.jrSelected]}
            onPress={() => setSelectedJr(i)}>
            <Text style={styles.jrDevEui}>DevEUI: {jr.devEui}</Text>
            <Text style={styles.jrDetail}>Nonce: {jr.devNonce} | MIC: {jr.mic}</Text>
          </TouchableOpacity>
        ))
      )}

      <Text style={styles.section}>Dictionary Attack</Text>
      <Text style={styles.hint}>Enter candidate keys (32-hex chars each, one per line)</Text>
      <TextInput
        style={styles.dictInput}
        value={dictHex}
        onChangeText={setDictHex}
        multiline
        numberOfLines={6}
        placeholder="00000000000000000000000000000000"
        placeholderTextColor="#555"
        editable={!running}
      />
      <TouchableOpacity style={styles.button} onPress={runDictionary} disabled={running || !joinReqs.length}>
        <Text style={styles.btnText}>{running ? 'Searching...' : 'Run Dictionary Attack'}</Text>
      </TouchableOpacity>

      <Text style={styles.section}>Single Key Test</Text>
      <TextInput
        style={styles.input}
        value={customKey}
        onChangeText={setCustomKey}
        placeholder="e.g. 00000000000000000000000000000000"
        placeholderTextColor="#555"
        maxLength={32}
        editable={!running}
      />
      <TouchableOpacity style={styles.button} onPress={runCustomKey} disabled={running || customKey.length !== 32}>
        <Text style={styles.btnText}>Test This Key</Text>
      </TouchableOpacity>

      {running && (
        <View style={styles.runningBox}>
          <ActivityIndicator size="large" color="#00ff88" />
          <Text style={styles.runningText}>Brute-forcing MIC on-device...</Text>
        </View>
      )}

      {keyResult && (
        <View style={[styles.resultBox, keyResult.found ? styles.foundBox : styles.notFoundBox]}>
          <Text style={[styles.resultTitle, keyResult.found ? styles.foundText : styles.notFoundText]}>
            {keyResult.found ? '✅ KEY FOUND!' : '❌ Key not found'}
          </Text>
          <Text style={styles.resultDetail}>Trials: {keyResult.trials.toLocaleString()}</Text>
          {keyResult.found && keyResult.key && (
            <>
              <Text style={styles.resultDetail}>AppKey/NwkKey: {keyResult.key}</Text>
              {keyResult.nwkSKey && <Text style={styles.resultDetail}>NwkSKey: {keyResult.nwkSKey}</Text>}
              {keyResult.appSKey && <Text style={styles.resultDetail}>AppSKey: {keyResult.appSKey}</Text>}
            </>
          )}
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 10 },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 15, marginBottom: 8 },
  hint: { color: '#666', fontSize: 11, marginBottom: 5 },
  empty: { color: '#888', fontSize: 13, fontStyle: 'italic' },
  jrItem: { backgroundColor: '#1a1a1a', borderRadius: 6, padding: 10, marginBottom: 5,
            borderWidth: 1, borderColor: '#333' },
  jrSelected: { borderColor: '#00ff88', backgroundColor: '#0a2a1a' },
  jrDevEui: { color: '#fff', fontSize: 13, fontWeight: 'bold' },
  jrDetail: { color: '#888', fontSize: 11, marginTop: 2 },
  dictInput: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
               borderRadius: 4, padding: 10, fontSize: 12, fontFamily: 'monospace',
               minHeight: 100, textAlignVertical: 'top' },
  input: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 10, fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  button: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
            borderRadius: 6, padding: 12, alignItems: 'center', marginTop: 5 },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: '600' },
  runningBox: { alignItems: 'center', marginTop: 20 },
  runningText: { color: '#aaa', marginTop: 10, fontSize: 13 },
  resultBox: { borderRadius: 8, padding: 15, marginTop: 15 },
  foundBox: { backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88' },
  notFoundBox: { backgroundColor: '#3a0a0a', borderWidth: 1, borderColor: '#ff6666' },
  resultTitle: { fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  foundText: { color: '#00ff88' },
  notFoundText: { color: '#ff6666' },
  resultDetail: { color: '#ccc', fontSize: 13, fontFamily: 'monospace', marginTop: 3 },
});