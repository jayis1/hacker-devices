// src/screens/InjectionScreen.js — ZCL / beacon / rejoin injection controls
// Author: jayis1
// License: MIT
import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Alert } from 'react-native';
import { useDevice } from '../context/DeviceContext';

export default function InjectionScreen() {
  const { commands, status } = useDevice();
  const [dstShort, setDstShort] = useState('0000');
  const [zclCmd, setZclCmd] = useState('2');   // default Toggle
  const [confirming, setConfirming] = useState(null);

  // Hold-to-confirm: requires two taps within 3 seconds
  function requireConfirm(action, label) {
    if (confirming === label) {
      action();
      setConfirming(null);
    } else {
      setConfirming(label);
      setTimeout(() => setConfirming(null), 3000);
    }
  }

  function doZclInject() {
    const dst = parseInt(dstShort, 16);
    const cmd = parseInt(zclCmd, 10);
    if (isNaN(dst) || isNaN(cmd)) { Alert.alert('Invalid parameters'); return; }
    requireConfirm(() => commands.injectZcl(dst, cmd)
      .then(() => Alert.alert('Injected', `ZCL cmd ${cmd} → 0x${dstShort}`))
      .catch((e) => Alert.alert('Error', String(e))), 'zcl');
  }

  function doBeacon() {
    requireConfirm(() => commands.injectBeacon()
      .then(() => Alert.alert('Rogue beacon transmitted'))
      .catch((e) => Alert.alert('Error', String(e))), 'beacon');
  }

  return (
    <View style={styles.container}>
      <Text style={styles.header}>⚠ Injection Controls</Text>
      <Text style={styles.warn}>
        Active injection. Ensure you have explicit written authorization
        before transmitting. All actions require hold-to-confirm.
      </Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>ZCL Cluster Command</Text>
        <Text style={styles.hint}>Target short addr (hex):</Text>
        <TextInput style={styles.input} value={dstShort} onChangeText={setDstShort} />
        <Text style={styles.hint}>ZCL command (0=Off, 1=On, 2=Toggle):</Text>
        <TextInput style={styles.input} value={zclCmd} onChangeText={setZclCmd} keyboardType="numeric" />
        <TouchableOpacity
          style={[styles.actionBtn, confirming === 'zcl' && styles.btnArmed]}
          onPress={doZclInject}
        >
          <Text style={styles.actionBtnText}>
            {confirming === 'zcl' ? 'TAP AGAIN TO CONFIRM' : 'Inject ZCL Command'}
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Rogue Coordinator Beacon</Text>
        <Text style={styles.hint}>
          Transmit a forged NWK beacon on CH{status.channel} advertising join-permitted.
          Devices in range may issue a Join Request.
        </Text>
        <TouchableOpacity
          style={[styles.actionBtn, confirming === 'beacon' && styles.btnArmed]}
          onPress={doBeacon}
        >
          <Text style={styles.actionBtnText}>
            {confirming === 'beacon' ? 'TAP AGAIN TO CONFIRM' : 'Transmit Rogue Beacon'}
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Rejoin Request Flood</Text>
        <Text style={styles.hint}>
          Broadcast forged Rejoin Requests to force key renegotiation on target mesh.
        </Text>
        <TouchableOpacity
          style={[styles.actionBtn, confirming === 'rejoin' && styles.btnArmed]}
          onPress={() => requireConfirm(() => Alert.alert('Not implemented in this build'), 'rejoin')}
        >
          <Text style={styles.actionBtnText}>
            {confirming === 'rejoin' ? 'TAP AGAIN TO CONFIRM' : 'Start Rejoin Flood'}
          </Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 12 },
  header: { color: '#f44336', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  warn: { color: '#ff9800', fontSize: 10, marginBottom: 16, lineHeight: 14 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 12 },
  cardTitle: { color: '#e85d2c', fontSize: 13, fontWeight: 'bold', marginBottom: 8 },
  hint: { color: '#888', fontSize: 10, marginTop: 4 },
  input: { backgroundColor: '#0f0f23', color: '#ddd', borderRadius: 4, padding: 8, marginTop: 4, marginBottom: 4, fontSize: 12 },
  actionBtn: { backgroundColor: '#e85d2c', padding: 12, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  btnArmed: { backgroundColor: '#f44336' },
  actionBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
});