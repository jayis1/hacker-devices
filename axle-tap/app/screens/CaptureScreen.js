// CaptureScreen.js — PCAP capture controls + protocol distribution
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import * as Ble from '../utils/ble';
import * as Proto from '../utils/protocol';

export default function CaptureScreen() {
  const [running, setRunning] = useState(false);
  const [captured, setCaptured] = useState(0);
  const [protoDist, setProtoDist] = useState({
    gPTP: 0, SOMEIP: 0, ARP: 0, DoIP: 0, AVB: 0, other: 0,
  });

  const send = async (frame, label, after) => {
    const ok = await Ble.sendCommand(frame);
    if (!ok) Alert.alert('Error', `Failed: ${label}`);
    else if (after) after();
  };

  return (
    <View style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.h2}>Passive tap</Text>
        <Text style={styles.text}>
          Mirror both directions of the automotive Ethernet link to the
          USB-CDC virtual Ethernet and SD card. Wireshark can read the
          stream as a live capture.
        </Text>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.btn, running && styles.active]}
            onPress={() => send(Proto.cmdTapStart(), 'tap on', () => setRunning(true))}
          >
            <Text style={styles.btnText}>Start tap</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.btn}
            onPress={() => send(Proto.cmdTapStop(), 'tap off', () => setRunning(false))}
          >
            <Text style={styles.btnText}>Stop tap</Text>
          </TouchableOpacity>
        </View>
        <Text style={styles.text}>{running ? '● Recording' : '○ Idle'} — frames: {captured}</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Protocol distribution</Text>
        {Object.entries(protoDist).map(([k, v]) => (
          <View key={k} style={styles.barRow}>
            <Text style={styles.barLabel}>{k}</Text>
            <View style={styles.barBg}>
              <View style={[styles.barFg, { width: `${Math.min(100, v / 10)}%` }]} />
            </View>
            <Text style={styles.barVal}>{v}</Text>
          </View>
        ))}
        <TouchableOpacity
          style={styles.btn}
          onPress={() => send(Proto.cmdStatus(), 'status')}
        >
          <Text style={styles.btnText}>Refresh</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>SD-card dump</Text>
        <Text style={styles.text}>
          Dump the SD-card PCAP ring to the USB-CDC stream. The laptop
          reads this as a continuous PCAP file.
        </Text>
        <TouchableOpacity
          style={styles.btn}
          onPress={() => Ble.sendCommand(Proto.encodeFrame(Proto.MSG.CMD, strToBytes('pcap dump'), null), 'pcap dump')}
        >
          <Text style={styles.btnText}>Dump SD to USB</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

function strToBytes(s) {
  const a = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) a[i] = s.charCodeAt(i);
  return a;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212' },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#2a2a2a' },
  row: { flexDirection: 'row', marginTop: 6 },
  h2: { color: '#7ab8ff', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  text: { color: '#ddd', fontSize: 12, fontFamily: 'monospace', marginVertical: 4 },
  btn: {
    backgroundColor: '#2a4d6e', paddingHorizontal: 14, paddingVertical: 8,
    borderRadius: 4, marginRight: 8, marginBottom: 8,
  },
  active: { borderColor: '#00ff66', borderWidth: 2 },
  btnText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
  barRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 3 },
  barLabel: { color: '#aaa', fontSize: 11, width: 70, fontFamily: 'monospace' },
  barBg: { flex: 1, height: 10, backgroundColor: '#2a2a2a', borderRadius: 3, marginRight: 8 },
  barFg: { height: 10, backgroundColor: '#7ab8ff', borderRadius: 3 },
  barVal: { color: '#ddd', fontSize: 11, width: 50, fontFamily: 'monospace' },
});