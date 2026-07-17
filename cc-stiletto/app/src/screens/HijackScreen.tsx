// src/screens/HijackScreen.tsx — automated DR/PR/FR swap fuzzer.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React, { useState } from 'react';
import { View, Text, TextInput, Pressable, StyleSheet, ScrollView, Switch } from 'react-native';
import { useDevice } from '../state/DeviceContext';
import { ConnectionBar } from '../components/ConnectionBar';

export default function HijackScreen() {
  const { send } = useDevice();
  const [dr, setDr] = useState(true);
  const [pr, setPr] = useState(false);
  const [frs, setFrs] = useState(false);
  const [interval, setInterval] = useState('2000');

  return (
    <ScrollView style={styles.container}>
      <ConnectionBar />
      <Text style={styles.h1}>Role Hijack Fuzzer</Text>
      <Text style={styles.help}>
        Issue unsolicited DR_Swap (data role), PR_Swap (power role), and/or
        Fast Role Swap signals at a set interval. A successful DR_Swap turns a
        "charger" into a USB host that can enumerate the victim.
      </Text>

      <View style={styles.row}>
        <Text style={styles.label}>DR_Swap (data role)</Text>
        <Switch value={dr} onValueChange={setDr} />
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>PR_Swap (power role)</Text>
        <Switch value={pr} onValueChange={setPr} />
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>FRS signal</Text>
        <Switch value={frs} onValueChange={setFrs} />
      </View>

      <Text style={styles.label}>Interval (ms)</Text>
      <TextInput style={styles.input} value={interval} onChangeText={setInterval} keyboardType="numeric" />

      <Pressable
        style={styles.btn}
        onPress={() => {
          send({ cmd: 'mode', mode: 'role-hijack' });
          send({ cmd: 'hijack', dr: dr ? 1 : 0, pr: pr ? 1 : 0, frs: frs ? 1 : 0, ms: parseInt(interval) || 2000 });
        }}
      >
        <Text style={styles.btnText}>Start Hijack</Text>
      </Pressable>

      <View style={{ height: 1, backgroundColor: '#333', marginVertical: 16 }} />

      <Text style={styles.h1}>VCONN Hijack</Text>
      <Text style={styles.help}>
        Steal the VCONN supply the host provides to power e-markers/cables. The
        device routes VCONN to its internal payload while the host thinks it is
        powering a passive cable.
      </Text>
      <Pressable
        style={[styles.btn, { backgroundColor: '#ffd60a' }]}
        onPress={() => send({ cmd: 'mode', mode: 'vconn-hijack' })}
      >
        <Text style={[styles.btnText, { color: '#000' }]}>Enable VCONN Hijack</Text>
      </Pressable>

      <View style={{ height: 1, backgroundColor: '#333', marginVertical: 16 }} />

      <Text style={styles.h1}>Dead-Battery Mode</Text>
      <Text style={styles.help}>
        Present as a dead-battery sink: pull CC down with Rd, wait for 5 V, then
        request minimum current. Useful to extract power from an unknown port or
        coerce a target into a vulnerable bootstrap state.
      </Text>
      <Pressable
        style={[styles.btn, { backgroundColor: '#3a86ff' }]}
        onPress={() => send({ cmd: 'mode', mode: 'dead-battery' })}
      >
        <Text style={styles.btnText}>Enter Dead-Battery Mode</Text>
      </Pressable>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d0d' },
  h1:        { color: '#eee', fontSize: 16, fontWeight: 'bold', padding: 12 },
  help:      { color: '#999', fontSize: 11, paddingHorizontal: 12, marginBottom: 8 },
  row:       { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', paddingHorizontal: 12, paddingVertical: 6 },
  label:     { color: '#bbb', fontSize: 13 },
  input:     { backgroundColor: '#1a1a1a', color: '#eee', margin: 12, padding: 10, borderRadius: 6, borderWidth: 1, borderColor: '#333' },
  btn:       { backgroundColor: '#4cc9f0', margin: 12, padding: 14, borderRadius: 8, alignItems: 'center' },
  btnText:   { color: 'white', fontWeight: 'bold' },
});