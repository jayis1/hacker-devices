/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — Discovery Screen
 *
 * Live pin-impedance scan visualization, protocol auto-detect with
 * confidence scoring, and channel mapping display.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert } from 'react-native';
import { Buffer } from 'buffer';

const CHANNEL_NAMES = [
  'CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7'
];
const CHANNEL_ROLES = [
  'SWCLK/TCK', 'SWDIO/TMS', 'TDI', 'TDO',
  'VRef Sense', 'Tgt Power', 'Continuity', 'GND Sense'
];

export default function DiscoveryScreen({ discovery, sendCommand, connect, connected }) {
  const [scanning, setScanning] = useState(false);

  const handleScan = useCallback(async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to GHOSTBUS first');
      return;
    }
    setScanning(true);
    await sendCommand(0x01, Buffer.from([3])); // discover all protocols
    setTimeout(() => setScanning(false), 6000);
  }, [connected, sendCommand]);

  const handlePowerOn = () => sendCommand(0x09, Buffer.alloc(0));
  const handlePowerOff = () => sendCommand(0x0A, Buffer.alloc(0));

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Debug Port Discovery</Text>
        <Text style={styles.desc}>
          GHOSTBUS auto-discovers SWD, JTAG, and cJTAG interfaces via
          impedance-guided pin scanning. Connect the 8 pogo-pin probe head
          to the target's test pads, then tap Discover.
        </Text>
        <TouchableOpacity style={[styles.button, scanning && styles.buttonScanning]}
                          onPress={handleScan} disabled={scanning}>
          <Text style={styles.buttonText}>
            {scanning ? 'Scanning...' : '🔍 Discover Debug Port'}
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Channel Map</Text>
        {CHANNEL_NAMES.map((name, i) => {
          const isGnd = discovery && discovery.gnd_ch === i;
          const isVref = discovery && discovery.vref_ch === i;
          const isSwdio = discovery && discovery.swdio_ch === i;
          const isSwclk = discovery && discovery.swclk_ch === i;
          const isTck = discovery && discovery.tck_ch === i;
          const isTms = discovery && discovery.tms_ch === i;
          const isTdi = discovery && discovery.tdi_ch === i;
          const isTdo = discovery && discovery.tdo_ch === i;
          const role = isGnd ? '⏚ GND' : isVref ? 'VRef' : isSwdio ? 'SWDIO' :
                       isSwclk ? 'SWCLK' : isTck ? 'TCK' : isTms ? 'TMS' :
                       isTdi ? 'TDI' : isTdo ? 'TDO' : CHANNEL_ROLES[i];
          return (
            <View key={i} style={styles.chRow}>
              <Text style={styles.chName}>{name}</Text>
              <Text style={styles.chRole}>{role}</Text>
            </View>
          );
        })}
      </View>

      {discovery && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Discovery Result</Text>
          <View style={styles.row}>
            <Text style={styles.label}>Protocol:</Text>
            <Text style={[styles.value, { color: '#00ff88' }]}>{discovery.protocol}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>IDCODE:</Text>
            <Text style={styles.valueMono}>{discovery.idcode}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>VRef:</Text>
            <Text style={styles.value}>{discovery.vref_mv} mV</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Confidence:</Text>
            <View style={styles.barContainer}>
              <View style={[styles.barFill,
                            { width: `${discovery.confidence}%` }]} />
              <Text style={styles.barText}>{discovery.confidence}%</Text>
            </View>
          </View>
        </View>
      )}

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Target Power</Text>
        <Text style={styles.desc}>
          Optional: inject 3.3V / 500mA into the target via CH5 for
          cold-boot flash extraction (current-limited, reverse-polarity
          protected).
        </Text>
        <View style={styles.rowButtons}>
          <TouchableOpacity style={[styles.smallBtn, { borderColor: '#00ff88' }]}
                            onPress={handlePowerOn}>
            <Text style={styles.smallBtnText}>Power ON</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.smallBtn, { borderColor: '#ff3333' }]}
                            onPress={handlePowerOff}>
            <Text style={[styles.smallBtnText, { color: '#ff3333' }]}>Power OFF</Text>
          </TouchableOpacity>
        </View>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginBottom: 12,
          borderWidth: 1, borderColor: '#2a2a4e' },
  cardTitle: { color: '#00ff88', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  desc: { color: '#aaa', fontSize: 13, marginBottom: 12, lineHeight: 18 },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  rowButtons: { flexDirection: 'row', gap: 8, marginTop: 8 },
  label: { color: '#888', fontSize: 14 },
  value: { color: '#e0e0e0', fontSize: 14, fontWeight: '500' },
  valueMono: { color: '#00ffaa', fontSize: 14, fontFamily: 'monospace', fontWeight: 'bold' },
  chRow: { flexDirection: 'row', justifyContent: 'space-between',
           paddingVertical: 6, borderBottomWidth: 1, borderBottomColor: '#2a2a4e' },
  chName: { color: '#00ff88', fontSize: 13, fontFamily: 'monospace', fontWeight: 'bold' },
  chRole: { color: '#aaa', fontSize: 13 },
  button: { backgroundColor: '#00ff88', borderRadius: 6, padding: 14,
            alignItems: 'center', marginTop: 8 },
  buttonScanning: { backgroundColor: '#555' },
  buttonText: { color: '#000', fontWeight: 'bold', fontSize: 14 },
  smallBtn: { flex: 1, borderWidth: 1, borderRadius: 6, padding: 10,
              alignItems: 'center', marginRight: 4 },
  smallBtnText: { color: '#00ff88', fontSize: 13, fontWeight: '600' },
  barContainer: { flex: 1, height: 20, backgroundColor: '#333', borderRadius: 4,
                  marginLeft: 12, position: 'relative', justifyContent: 'center' },
  barFill: { position: 'absolute', left: 0, top: 0, bottom: 0,
             backgroundColor: '#00ff88', borderRadius: 4 },
  barText: { position: 'absolute', right: 8, color: '#fff', fontSize: 12,
             fontWeight: 'bold' },
});