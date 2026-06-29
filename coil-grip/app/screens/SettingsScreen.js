/*
 * SettingsScreen.js — BLE pairing, encryption, safety unlock, firmware update
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Alert } from 'react-native';
import { Card, Title, Paragraph, Button, TextInput, Divider, Switch } from 'react-native-paper';
import { useDevice } from '../components/DeviceContext';

const FOD_UNLOCK_CODE = '0xC01DABAD'; // mirror firmware constant

export default function SettingsScreen() {
  const { connected, sendCommand, connect, disconnect } = useDevice();
  const [pairingKey, setPairingKey] = useState('');
  const [safetyAck, setSafetyAck] = useState(false);
  const [safetyCode, setSafetyCode] = useState('');

  const pair = () => {
    if (pairingKey.length !== 64) { Alert.alert('Key must be 64 hex chars'); return; }
    connect();
    // In a full build we'd send the key over a bootstrap channel; here we
    // just store it locally and rely on the BLE manager's pairing.
  };

  const unlockFod = () => {
    if (!safetyAck) { Alert.alert('Acknowledge the safety warning first'); return; }
    if (safetyCode !== FOD_UNLOCK_CODE) { Alert.alert('Wrong code'); return; }
    sendCommand(`fod unlock 8192\n`); // 0x2000 decimal of the code's low bits
    Alert.alert('FOD safety-research mode UNLOCKED');
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>BLE pairing</Title>
          <Paragraph>Status: {connected ? 'connected' : 'disconnected'}</Paragraph>
          <TextInput
            label="ECDH P-256 pairing key (hex, 64 chars)"
            value={pairingKey}
            onChangeText={setPairingKey}
            style={styles.input}
          />
          <View style={styles.row}>
            <Button mode="contained" onPress={pair}>Pair & connect</Button>
            <Button mode="outlined" onPress={disconnect}>Disconnect</Button>
          </View>
          <Divider style={styles.div}/>
          <Title style={styles.h2}>Encryption</Title>
          <Paragraph>AES-256-CTR session key derived from ECDH handshake. Frames carry a 16-bit sequence and CRC-16.</Paragraph>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Safety research — FOD</Title>
          <Paragraph style={styles.warn}>⚠ Unlocking FOD manipulation lets a receiver hide a foreign object from the charger's safety check. This can cause fire. Only for authorized lab demonstrations with thermal monitoring.</Paragraph>
          <View style={styles.row}>
            <Text style={styles.label}>I understand the risk</Text>
            <Switch value={safetyAck} onValueChange={setSafetyAck}/>
          </View>
          <TextInput
            label="Unlock code"
            value={safetyCode}
            onChangeText={setSafetyCode}
            style={styles.input}
          />
          <Button mode="contained" color="#d73a49" onPress={unlockFod}>Unlock FOD research</Button>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>About</Title>
          <Paragraph>CoilGrip v1.0 — Wireless Power Transfer Manipulation Platform</Paragraph>
          <Paragraph>Author: jayis1</Paragraph>
          <Paragraph>License: GPL-2.0</Paragraph>
          <Paragraph style={styles.disclaim}>For authorized security research only. Misuse may violate safety and computer-fraud laws. Always obtain written authorization.</Paragraph>
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex:1, padding:12, backgroundColor:'#0d1117' },
  card: { marginBottom:12, backgroundColor:'#161b22' },
  row: { flexDirection:'row', alignItems:'center', marginVertical:6, gap:8 },
  input: { backgroundColor:'#0d1117', marginVertical:6 },
  div: { marginVertical:8 },
  h2: { fontSize:14, color:'#8b949e' },
  label: { color:'#c9d1d9' },
  warn: { color:'#d73a49', marginBottom:8 },
  disclaim: { color:'#8b949e', fontStyle:'italic', marginTop:8, fontSize:11 },
});