// screens/TunnelScreen.js — air-gap tunnel configuration + stats
// Author: jayis1
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, Alert } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import { MSG } from '../utils/protocol';

export default function TunnelScreen() {
  const { tunnel, send } = useReaper();
  const [peer, setPeer] = useState('');
  const [keyHex, setKeyHex] = useState('');

  const connect = () => {
    const peerBytes = peer ? peer.split(':').map((h) => parseInt(h, 16)) : null;
    let keyBytes = null;
    try { keyBytes = new Uint8Array(keyHex.match(/.{2}/g).map((h) => parseInt(h, 16))); }
    catch { Alert.alert('Bad key (hex, 32 bytes)'); return; }
    if (keyBytes.length !== 32) { Alert.alert('Key must be 32 bytes'); return; }
    const p = new Uint8Array(6 + 32);
    if (peerBytes) p.set(peerBytes.slice(0, 6), 0);
    p.set(keyBytes, 6);
    send(MSG.TUNNEL_CONNECT, p);
    Alert.alert('Tunnel', 'Tunnel connect command sent');
  };

  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.header}>
        <Text style={styles.h1}>Air-Gap Tunnel</Text>
        <Text style={styles.sub}>Bridge two Powerline-Reapers over the PLC segment</Text>
      </View>

      <View style={styles.statusBox}>
        <Text style={styles.h2}>Tunnel Status</Text>
        <Row label="State"     value={tunnel.up ? 'UP' : 'DOWN'} color={tunnel.up ? '#00ffaa' : '#ff5555'} />
        <Row label="Peer"      value={tunnel.peer || '—'} />
        <Row label="RX bytes" value={String(tunnel.rxBytes)} />
        <Row label="TX bytes" value={String(tunnel.txBytes)} />
      </View>

      <View style={styles.inputBox}>
        <Text style={styles.label}>Peer MAC (xx:xx:xx:xx:xx:xx)</Text>
        <TextInput style={styles.input} value={peer} onChangeText={setPeer}
          placeholder="02:1a:2b:..." placeholderTextColor="#444" />
        <Text style={styles.label}>Session Key (64 hex chars = 32 bytes)</Text>
        <TextInput style={[styles.input, styles.inputLong]} value={keyHex} onChangeText={setKeyHex}
          placeholder="a1b2c3..." placeholderTextColor="#444" multiline />
        <TouchableOpacity style={styles.btn} onPress={connect}>
          <Text style={styles.btnText}>Connect Tunnel</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.note}>
        Each frame sent through the tunnel is encrypted with ChaCha20-Poly1305
        using the session key, then carried over the PLC segment to the peer
        device. This bridges what the facility believes is an impenetrable
        physical air gap, using the shared electrical phase as the channel.
      </Text>
    </View>
  );
}

const Row = ({ label, value, color = '#eee' }) => (
  <View style={styles.row}><Text style={styles.rowLabel}>{label}</Text><Text style={[styles.rowVal, { color }]}>{value}</Text></View>
);

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  header: { marginVertical: 12 },
  h1: { color: '#eee', fontSize: 20, fontWeight: 'bold' },
  h2: { color: '#00ffaa', fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  sub: { color: '#888', fontSize: 12 },
  statusBox: { backgroundColor: '#111', padding: 12, borderRadius: 6, marginBottom: 12 },
  inputBox: { backgroundColor: '#111', padding: 12, borderRadius: 6, marginBottom: 12 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  rowLabel: { color: '#888', fontSize: 12, fontFamily: 'monospace' },
  rowVal: { color: '#eee', fontSize: 12, fontFamily: 'monospace' },
  label: { color: '#888', fontSize: 11, marginBottom: 4, marginTop: 6 },
  input: { backgroundColor: '#0a0a0a', color: '#eee', borderWidth: 1, borderColor: '#222', borderRadius: 4, padding: 8, fontFamily: 'monospace' },
  inputLong: { minHeight: 50 },
  btn: { backgroundColor: '#1a3a1a', padding: 14, borderRadius: 6, marginTop: 10, alignItems: 'center' },
  btnText: { color: '#00ffaa', fontSize: 14, fontWeight: 'bold' },
  note: { color: '#666', fontSize: 11, lineHeight: 16, marginTop: 12 },
});