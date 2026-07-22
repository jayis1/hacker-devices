// SomeipScreen.js — SOME/IP service discovery + fuzzer controls
// Author: jayis1
// License: MIT
// Date:   2026-07-22

import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, FlatList, Alert } from 'react-native';
import * as Ble from '../utils/ble';
import * as Proto from '../utils/protocol';

const FUZZ_MODES = ['trunc', 'oversize', 'badlen', 'msgid', 'badver', 'random'];

export default function SomeipScreen() {
  const [services, setServices] = useState([]);
  const [fuzzSvc, setFuzzSvc] = useState('0x1234');
  const [fuzzMethod, setFuzzMethod] = useState('0x0001');
  const [fuzzMode, setFuzzMode] = useState('trunc');
  const [fuzzing, setFuzzing] = useState(false);

  const send = async (frame, label, after) => {
    const ok = await Ble.sendCommand(frame);
    if (!ok) Alert.alert('Error', `Failed: ${label}`);
    else if (after) after();
  };

  const discover = () => {
    send(Proto.cmdSomeipDiscover(), 'someip discover');
    // The device reports services via the SVC_MAP telemetry message; a
    // real app would parse it. For the demo we just refresh from status.
    setTimeout(() => send(Proto.cmdStatus(), 'status'), 1000);
  };

  return (
    <View style={styles.container}>
      <View style={styles.section}>
        <Text style={styles.h2}>Service Discovery (SOME/IP-SD)</Text>
        <Text style={styles.text}>
          Send a SOME/IP-SD Find message to discover all services on the
          link. Discovered services are listed below.
        </Text>
        <TouchableOpacity style={styles.btn} onPress={discover}>
          <Text style={styles.btnText}>Discover services</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Discovered services</Text>
        <FlatList
          data={services}
          keyExtractor={(item, i) => i.toString()}
          renderItem={({ item }) => (
            <View style={styles.svcRow}>
              <Text style={styles.text}>
                {item.service_id}:{item.instance_id} port={item.port} proto={item.protocol}
              </Text>
            </View>
          )}
          ListEmptyComponent={<Text style={styles.text}>No services discovered yet.</Text>}
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.h2}>Fuzzer</Text>
        <Text style={styles.text}>
          Generate malformed SOME/IP payloads against a discovered service.
        </Text>
        <View style={styles.row}>
          <TextInput
            style={styles.input}
            value={fuzzSvc}
            onChangeText={setFuzzSvc}
            placeholder="svc hex"
          />
          <TextInput
            style={styles.input}
            value={fuzzMethod}
            onChangeText={setFuzzMethod}
            placeholder="method hex"
          />
        </View>
        <View style={styles.row}>
          {FUZZ_MODES.map((m) => (
            <TouchableOpacity
              key={m}
              style={[styles.modeBtn, fuzzMode === m && styles.activeMode]}
              onPress={() => setFuzzMode(m)}
            >
              <Text style={styles.modeText}>{m}</Text>
            </TouchableOpacity>
          ))}
        </View>
        <View style={styles.row}>
          <TouchableOpacity
            style={[styles.btn, styles.danger]}
            onPress={() => {
              const svc = fuzzSvc.replace('0x', '');
              const m = fuzzMethod.replace('0x', '');
              send(Proto.cmdSomeipFuzz(svc, m, fuzzMode), 'fuzz start', () => setFuzzing(true));
            }}
          >
            <Text style={styles.btnText}>Start fuzz</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.btn}
            onPress={() => send(Proto.cmdSomeipStop(), 'fuzz stop', () => setFuzzing(false))}
          >
            <Text style={styles.btnText}>Stop fuzz</Text>
          </TouchableOpacity>
        </View>
        <Text style={styles.text}>{fuzzing ? `Fuzzing ${fuzzSvc}/${fuzzMethod} mode=${fuzzMode}` : 'Idle'}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#121212' },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#2a2a2a' },
  row: { flexDirection: 'row', flexWrap: 'wrap', alignItems: 'center', marginTop: 6 },
  h2: { color: '#7ab8ff', fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  text: { color: '#ddd', fontSize: 12, fontFamily: 'monospace', marginVertical: 3 },
  btn: {
    backgroundColor: '#2a4d6e', paddingHorizontal: 14, paddingVertical: 8,
    borderRadius: 4, marginRight: 8, marginBottom: 8,
  },
  danger: { backgroundColor: '#aa3333' },
  btnText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
  input: {
    borderWidth: 1, borderColor: '#444', color: '#eee',
    paddingHorizontal: 10, paddingVertical: 6, borderRadius: 4,
    marginRight: 8, marginBottom: 8, width: 90, fontFamily: 'monospace',
  },
  modeBtn: {
    backgroundColor: '#333', paddingHorizontal: 8, paddingVertical: 5,
    borderRadius: 3, marginRight: 5, marginBottom: 5, borderWidth: 1, borderColor: '#555',
  },
  activeMode: { borderColor: '#ffaa00', backgroundColor: '#553300' },
  modeText: { color: '#eee', fontSize: 11, fontFamily: 'monospace' },
  svcRow: { paddingVertical: 4, borderBottomWidth: 1, borderBottomColor: '#222' },
});