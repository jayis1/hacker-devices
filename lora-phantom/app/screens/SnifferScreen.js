/*
 * lora-phantom / app / screens / SnifferScreen.js
 * Live decoded LoRaWAN frame list with region/DR/frequency filters.
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, FlatList, Switch } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, REGIONS, REGION_NAMES, buildSniffStart, parseFrame, parseJoinRequest } from '../utils/protocol';

export default function SnifferScreen() {
  const { connected, frames, joinReqs, sendCommand, clearFrames, status } = useDevice();
  const [freq, setFreq] = useState('868.1');
  const [sf, setSf] = useState('7');
  const [bw, setBw] = useState('125');
  const [sniffing, setSniffing] = useState(false);
  const [showJoinReqs, setShowJoinReqs] = useState(true);
  const [showDataFrames, setShowDataFrames] = useState(true);
  const flatListRef = useRef(null);

  useEffect(() => {
    if (status) {
      setFreq((status.sniffFreq / 1e6).toFixed(1));
      setSf(String(status.sniffSf));
    }
  }, [status]);

  const startSniff = () => {
    const freqHz = Math.round(parseFloat(freq) * 1e6);
    const sfVal = parseInt(sf, 10);
    const bwVal = bw === '125' ? 0 : bw === '250' ? 1 : 2;
    sendCommand(CMD.SNIFF_START, buildSniffStart(freqHz, sfVal, bwVal));
    setSniffing(true);
  };

  const stopSniff = () => {
    sendCommand(CMD.SNIFF_STOP);
    setSniffing(false);
  };

  const combined = [];
  if (showJoinReqs) {
    joinReqs.forEach((jr, i) => combined.push({ type: 'join', data: jr, key: 'j' + i }));
  }
  if (showDataFrames) {
    frames.forEach((f, i) => combined.push({ type: 'frame', data: f, key: 'f' + i }));
  }

  const renderItem = ({ item }) => {
    if (item.type === 'join') {
      const jr = item.data;
      return (
        <View style={styles.joinItem}>
          <Text style={styles.joinHdr}>🔗 JoinRequest</Text>
          <Text style={styles.itemDetail}>JoinEUI: {jr.joinEui}</Text>
          <Text style={styles.itemDetail}>DevEUI:  {jr.devEui}</Text>
          <Text style={styles.itemDetail}>DevNonce: {jr.devNonce}</Text>
          <Text style={styles.itemDetail}>MIC: {jr.mic}</Text>
          {jr.rssi !== undefined && (
            <Text style={styles.meta}>RSSI: {jr.rssi}dBm | SNR: {jr.snr} | SF{jr.sf} | {(jr.freqHz/1e6).toFixed(1)}MHz</Text>
          )}
        </View>
      );
    }
    const f = item.data;
    return (
      <View style={styles.frameItem}>
        <Text style={[styles.frameHdr, { color: f.validMic ? '#00ff88' : '#ff6666' }]}>
          {f.mtypeName} | DevAddr: {f.devAddr}
        </Text>
        <Text style={styles.itemDetail}>FCnt: {f.fcnt} | FPort: {f.fport} | FCtrl: {f.fctrl}</Text>
        <Text style={styles.itemDetail}>Payload ({f.payloadLen}B): {f.payloadHex.substring(0, 64)}{f.payloadLen > 32 ? '...' : ''}</Text>
        <Text style={styles.meta}>RSSI: {f.rssi}dBm | SNR: {f.snr} | SF{f.sf} | {(f.freqHz/1e6).toFixed(1)}MHz</Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.header}>📡 LoRaWAN Sniffer</Text>

      {/* Config Row */}
      <View style={styles.configRow}>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Freq (MHz)</Text>
          <TextInput style={styles.input} value={freq} onChangeText={setFreq}
            keyboardType="decimal-pad" editable={!sniffing} />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>SF</Text>
          <TextInput style={styles.input} value={sf} onChangeText={setSf}
            keyboardType="number-pad" editable={!sniffing} />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>BW (kHz)</Text>
          <TextInput style={styles.input} value={bw} onChangeText={setBw}
            keyboardType="number-pad" editable={!sniffing} />
        </View>
      </View>

      {/* Toggle Row */}
      <View style={styles.toggleRow}>
        <View style={styles.toggle}>
          <Text style={styles.toggleLabel}>JoinReqs</Text>
          <Switch value={showJoinReqs} onValueChange={setShowJoinReqs}
            trackColor={{ true: '#00ff88', false: '#333' }} />
        </View>
        <View style={styles.toggle}>
          <Text style={styles.toggleLabel}>Data Frames</Text>
          <Switch value={showDataFrames} onValueChange={setShowDataFrames}
            trackColor={{ true: '#00ff88', false: '#333' }} />
        </View>
      </View>

      {/* Start/Stop */}
      <View style={styles.btnRow}>
        {!sniffing ? (
          <TouchableOpacity style={styles.startBtn} onPress={startSniff}>
            <Text style={styles.btnText}>▶ Start Sniffing</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.stopBtn} onPress={stopSniff}>
            <Text style={styles.btnText}>⏹ Stop</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={styles.clearBtn} onPress={clearFrames}>
          <Text style={styles.btnText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.countText}>{combined.length} frames captured</Text>

      {/* Frame List */}
      <FlatList
        ref={flatListRef}
        data={combined}
        renderItem={renderItem}
        keyExtractor={item => item.key}
        style={styles.list}
        onContentSizeChange={() => flatListRef.current?.scrollToEnd()}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 10 },
  configRow: { flexDirection: 'row', gap: 8, marginBottom: 10 },
  configItem: { flex: 1 },
  configLabel: { color: '#888', fontSize: 11, marginBottom: 3 },
  input: { backgroundColor: '#1a1a1a', color: '#fff', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 8, fontSize: 14 },
  toggleRow: { flexDirection: 'row', gap: 20, marginBottom: 10 },
  toggle: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  toggleLabel: { color: '#ccc', fontSize: 13 },
  btnRow: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  startBtn: { flex: 1, backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88',
              borderRadius: 6, padding: 12, alignItems: 'center' },
  stopBtn: { flex: 1, backgroundColor: '#3a0a0a', borderWidth: 1, borderColor: '#ff6666',
             borderRadius: 6, padding: 12, alignItems: 'center' },
  clearBtn: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#666',
              borderRadius: 6, padding: 12 },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: '600' },
  countText: { color: '#888', fontSize: 12, marginBottom: 5 },
  list: { flex: 1 },
  joinItem: { backgroundColor: '#1a1a2a', borderRadius: 6, padding: 10, marginBottom: 5,
              borderLeftWidth: 3, borderLeftColor: '#8866ff' },
  frameItem: { backgroundColor: '#1a1a1a', borderRadius: 6, padding: 10, marginBottom: 5,
               borderLeftWidth: 3, borderLeftColor: '#00ff88' },
  joinHdr: { color: '#8866ff', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  frameHdr: { fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  itemDetail: { color: '#aaa', fontSize: 11, lineHeight: 16 },
  meta: { color: '#666', fontSize: 10, marginTop: 3 },
});