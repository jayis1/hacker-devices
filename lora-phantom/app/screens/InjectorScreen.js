/*
 * lora-phantom / app / screens / InjectorScreen.js
 * Craft and transmit downlink frames (MAC commands, app payloads, JoinAccept).
 * Author: jayis1
 * License: MIT
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView, Picker } from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { CMD, buildInjectPayload, hexToBytes } from '../utils/protocol';

export default function InjectorScreen() {
  const { connected, status, sendCommand } = useDevice();
  const [devAddr, setDevAddr] = useState('26010101');
  const [fctrl, setFctrl] = useState('00');
  const [fcnt, setFcnt] = useState('0');
  const [fport, setFport] = useState('1');
  const [confirmed, setConfirmed] = useState(false);
  const [nwkSKey, setNwkSKey] = useState('00000000000000000000000000000000');
  const [appSKey, setAppSKey] = useState('00000000000000000000000000000000');
  const [payload, setPayload] = useState('');
  const [macCommand, setMacCommand] = useState('none');
  const [result, setResult] = useState(null);

  const inject = () => {
    const devAddrInt = parseInt(devAddr, 16);
    const fcntInt = parseInt(fcnt, 10);
    const fportInt = parseInt(fport, 10);
    const nwkKeyBytes = hexToBytes(nwkSKey);
    const appKeyBytes = hexToBytes(appSKey);
    let appPayload = new Uint8Array(0);
    if (payload.length > 0) appPayload = hexToBytes(payload);
    const p = buildInjectPayload(devAddrInt, parseInt(fctrl, 16), fcntInt, fportInt,
                                  confirmed, nwkKeyBytes, appKeyBytes, appPayload);
    sendCommand(CMD.INJECT_SEND, p);
    setResult('Sent — awaiting confirmation...');
  };

  const injectMacCommand = () => {
    // MAC commands go as FPort=0 payload, encrypted with NwkSKey
    const cmdMap = {
      dev_status: { cmd: 0x06, payload: '' },       // DevStatusReq
      link_adr: { cmd: 0x03, payload: '00FF0000' }, // LinkADRReq: DR0, TXPower=0, ChMask=0xFF00
      duty_cycle: { cmd: 0x04, payload: '00' },     // DutyCycleReq: 0%
      rx_param: { cmd: 0x05, payload: '000000' },   // RxParamSetupReq
    };
    const cmd = cmdMap[macCommand];
    if (!cmd) return;
    const macPayload = new Uint8Array(1 + cmd.payload.length / 2);
    macPayload[0] = cmd.cmd;
    if (cmd.payload) {
      const bytes = hexToBytes(cmd.payload);
      macPayload.set(bytes, 1);
    }
    const devAddrInt = parseInt(devAddr, 16);
    const fcntInt = parseInt(fcnt, 10);
    const nwkKeyBytes = hexToBytes(nwkSKey);
    const p = buildInjectPayload(devAddrInt, 0x00, fcntInt, 0,
                                  false, nwkKeyBytes, nwkKeyBytes, macPayload);
    sendCommand(CMD.INJECT_SEND, p);
    setResult('MAC command sent: ' + macCommand);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>📤 Downlink Injector</Text>

      <Text style={styles.section}>Target Device</Text>
      <View style={styles.row}>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>DevAddr (hex)</Text>
          <TextInput style={styles.input} value={devAddr} onChangeText={setDevAddr}
            maxLength={8} />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>FCnt</Text>
          <TextInput style={styles.input} value={fcnt} onChangeText={setFcnt}
            keyboardType="number-pad" />
        </View>
      </View>

      <View style={styles.row}>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>FCtrl (hex)</Text>
          <TextInput style={styles.input} value={fctrl} onChangeText={setFctrl}
            maxLength={2} />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>FPort</Text>
          <TextInput style={styles.input} value={fport} onChangeText={setFport}
            keyboardType="number-pad" />
        </View>
        <View style={styles.configItem}>
          <Text style={styles.configLabel}>Confirmed</Text>
          <TouchableOpacity style={[styles.toggle, confirmed && styles.toggleOn]}
            onPress={() => setConfirmed(!confirmed)}>
            <Text style={styles.toggleText}>{confirmed ? 'YES' : 'NO'}</Text>
          </TouchableOpacity>
        </View>
      </View>

      <Text style={styles.section}>Session Keys</Text>
      <TextInput style={styles.keyInput} value={nwkSKey} onChangeText={setNwkSKey}
        maxLength={32} placeholder="NwkSKey (32 hex)" placeholderTextColor="#555" />
      <TextInput style={styles.keyInput} value={appSKey} onChangeText={setAppSKey}
        maxLength={32} placeholder="AppSKey (32 hex)" placeholderTextColor="#555" />

      <Text style={styles.section}>Application Payload (hex)</Text>
      <TextInput style={styles.payloadInput} value={payload} onChangeText={setPayload}
        placeholder="e.g. 0102AABB" placeholderTextColor="#555" multiline />

      <TouchableOpacity style={styles.injectBtn} onPress={inject} disabled={!connected}>
        <Text style={styles.btnText}>⚡ Inject Downlink</Text>
      </TouchableOpacity>

      <Text style={styles.section}>MAC Command Injection</Text>
      <View style={styles.pickerRow}>
        <Picker
          selectedValue={macCommand}
          onValueChange={setMacCommand}
          style={styles.picker}
          dropdownIconColor="#00ff88">
          <Picker.Item label="Select..." value="none" color="#aaa" />
          <Picker.Item label="DevStatusReq" value="dev_status" color="#aaa" />
          <Picker.Item label="LinkADRReq" value="link_adr" color="#aaa" />
          <Picker.Item label="DutyCycleReq" value="duty_cycle" color="#aaa" />
          <Picker.Item label="RxParamSetupReq" value="rx_param" color="#aaa" />
        </Picker>
        <TouchableOpacity style={styles.macBtn} onPress={injectMacCommand} disabled={macCommand === 'none'}>
          <Text style={styles.btnText}>Send MAC Cmd</Text>
        </TouchableOpacity>
      </View>

      {result && (
        <View style={styles.resultBox}>
          <Text style={styles.resultText}>{result}</Text>
        </View>
      )}

      <View style={styles.warning}>
        <Text style={styles.warningText}>
          ⚠️ Downlink injection transmits on LoRa frequencies. Ensure
          regulatory compliance (duty cycle, power limits) and authorization.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 15 },
  header: { fontSize: 20, fontWeight: 'bold', color: '#00ff88', marginBottom: 10 },
  section: { fontSize: 15, fontWeight: 'bold', color: '#ccc', marginTop: 15, marginBottom: 6 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 6 },
  configItem: { flex: 1 },
  configLabel: { color: '#888', fontSize: 11, marginBottom: 3 },
  input: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
           borderRadius: 4, padding: 8, fontSize: 13, fontFamily: 'monospace' },
  keyInput: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
              borderRadius: 4, padding: 10, fontSize: 13, fontFamily: 'monospace', marginBottom: 6 },
  payloadInput: { backgroundColor: '#1a1a1a', color: '#00ff88', borderWidth: 1, borderColor: '#333',
                  borderRadius: 4, padding: 10, fontSize: 13, fontFamily: 'monospace',
                  minHeight: 50, textAlignVertical: 'top' },
  toggle: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#333',
            borderRadius: 4, padding: 10, alignItems: 'center' },
  toggleOn: { borderColor: '#00ff88', backgroundColor: '#0a3a1a' },
  toggleText: { color: '#ccc', fontSize: 13, fontWeight: '600' },
  injectBtn: { backgroundColor: '#0a3a1a', borderWidth: 1, borderColor: '#00ff88',
               borderRadius: 6, padding: 14, alignItems: 'center', marginTop: 10 },
  btnText: { color: '#00ff88', fontSize: 14, fontWeight: 'bold' },
  pickerRow: { flexDirection: 'row', gap: 8, alignItems: 'center' },
  picker: { flex: 1, backgroundColor: '#1a1a1a', color: '#ccc', borderRadius: 4 },
  macBtn: { backgroundColor: '#1a1a1a', borderWidth: 1, borderColor: '#00ff88',
            borderRadius: 4, padding: 12 },
  resultBox: { backgroundColor: '#0a2a1a', borderRadius: 6, padding: 10, marginTop: 10,
               borderWidth: 1, borderColor: '#00ff88' },
  resultText: { color: '#00ff88', fontSize: 13 },
  warning: { backgroundColor: '#2a1a0a', borderWidth: 1, borderColor: '#ff6600',
             borderRadius: 6, padding: 10, marginTop: 15 },
  warningText: { color: '#ffaa44', fontSize: 11 },
});