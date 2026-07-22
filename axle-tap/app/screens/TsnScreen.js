// TsnScreen.js — TSN (802.1CB / 802.1Qbv) manipulation controls
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import * as Ble from '../utils/ble';
import * as Proto from '../utils/protocol';

export default function TsnScreen() {
  const [seqForgeMode, setSeqForgeMode] = useState('off');
  const [srSpoofOn, setSrSpoofOn] = useState(false);

  const send = async (frame, label, after) => {
    const ok = await Ble.sendCommand(frame);
    if (!ok) Alert.alert('Error', `Failed: ${label}`);
    else if (after) after();
  };

  return (
    <View style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.h2}>802.1CB sequence-number forgery</Text>
        <Text style={styles.text}>
          Forge the sequence-number field of 802.1CB-tagged frames to
          trigger duplicate-discard or out-of-sequence handling at the
          receiver's stream splitter.
        </Text>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.btn, styles.danger, seqForgeMode === 'dup' && styles.active]}
            onPress={() => send(Proto.cmdTsnSeqForgeDup(), 'CB dup', () => setSeqForgeMode('dup'))}
          >
            <Text style={styles.btnText}>Forge duplicate</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.btn, styles.danger, seqForgeMode === 'out' && styles.active]}
            onPress={() => send(Proto.cmdTsnSeqForgeOut(), 'CB out', () => setSeqForgeMode('out'))}
          >
            <Text style={styles.btnText}>Forge out-of-order</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.btn}
            onPress={() => send(Proto.cmdTsnSeqForgeOff(), 'CB off', () => setSeqForgeMode('off'))}
          >
            <Text style={styles.btnText}>Off</Text>
          </TouchableOpacity>
        </View>
        <Text style={styles.text}>Mode: {seqForgeMode}</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>802.1Qbv out-of-window injection</Text>
        <Text style={styles.text}>
          Inject a frame into a time slot whose gate is closed for the
          matching SR-class, causing a 802.1Qbv schedule collision.
        </Text>
        <TouchableOpacity style={[styles.btn, styles.danger]} onPress={() => send(Proto.cmdTsnQbvInject(), 'QBV inject')}>
          <Text style={styles.btnText}>Inject out-of-window frame</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>SR-class reservation spoof</Text>
        <Text style={styles.text}>
          Claim SR-A bandwidth via forged MSRP Talker_Advertise messages
          to starve best-effort traffic.
        </Text>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.btn, styles.danger, srSpoofOn && styles.active]}
            onPress={() => send(Proto.cmdTsnSrSpoof(), 'SR spoof', () => setSrSpoofOn(true))}
          >
            <Text style={styles.btnText}>Spoof SR-A</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.btn}
            onPress={() => send(Proto.cmdTsnSrOff(), 'SR off', () => setSrSpoofOn(false))}
          >
            <Text style={styles.btnText}>Off</Text>
          </TouchableOpacity>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212' },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#2a2a2a' },
  row: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 6 },
  h2: { color: '#7ab8ff', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  text: { color: '#ddd', fontSize: 12, fontFamily: 'monospace', marginVertical: 4 },
  btn: {
    backgroundColor: '#2a4d6e', paddingHorizontal: 14, paddingVertical: 8,
    borderRadius: 4, marginRight: 8, marginBottom: 8,
  },
  danger: { backgroundColor: '#aa3333' },
  active: { borderColor: '#ffaa00', borderWidth: 2 },
  btnText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
});