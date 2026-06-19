/**
 * DFAScreen.tsx — live DFA key recovery display
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { useBle } from '../services/BleContext';

export default function DFAScreen() {
  const ble = useBle();
  const dfa = ble.dfaResult;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>AES-128 Piret-Quisquater DFA</Text>
      <Text style={styles.subtitle}>On-device differential fault analysis</Text>

      <View style={styles.card}>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Faulted pairs:</Text>
          <Text style={styles.v}>{dfa?.faultsCollected ?? 0}</Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Unique key bytes:</Text>
          <Text style={[styles.v, { color: (dfa?.uniqueBytes ?? 0) === 16 ? '#4ecca3' : '#e94560' }]}>
            {dfa?.uniqueBytes ?? 0} / 16
          </Text>
        </View>
      </View>

      <Text style={styles.section}>Recovered Last-Round Key</Text>
      <View style={styles.keyGrid}>
        {(dfa?.key ?? new Array(16).fill(-1)).map((b, i) => (
          <View key={i} style={[styles.keyCell, b >= 0 ? styles.keyKnown : styles.keyUnknown]}>
            <Text style={styles.keyIdx}>{i}</Text>
            <Text style={styles.keyVal}>{b >= 0 ? b.toString(16).padStart(2, '0').toUpperCase() : '??'}</Text>
          </View>
        ))}
      </View>

      {dfa && dfa.uniqueBytes === 16 && (
        <View style={styles.fullKeyBox}>
          <Text style={styles.fullKeyLabel}>Full AES-128 Key (hex):</Text>
          <Text style={styles.fullKey}>
            {dfa.key.map((b) => b.toString(16).padStart(2, '0')).join('')}
          </Text>
          <Text style={styles.note}>
            Apply the inverse AES key schedule to derive the master key.
          </Text>
        </View>
      )}

      <Text style={styles.help}>
        How it works: PhotonStrike injects single-byte faults into the AES
        state just before the last MixColumns. Each faulted ciphertext pair
        constrains 4 last-round-key bytes. After 4 clean pairs in different
        columns, all 16 bytes are recovered. The solver runs on the device's
        Cortex-M7 in milliseconds.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  title: { color: '#e94560', fontSize: 22, fontWeight: 'bold' },
  subtitle: { color: '#a0a0c0', fontSize: 13, marginBottom: 16 },
  card: { backgroundColor: '#16213e', borderRadius: 10, padding: 16, marginBottom: 12 },
  kvRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 3 },
  k: { color: '#a0a0c0' },
  v: { color: '#fff', fontWeight: '700' },
  section: { color: '#e94560', fontSize: 14, fontWeight: '700', marginBottom: 8 },
  keyGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  keyCell: {
    width: '23%', backgroundColor: '#16213e', borderRadius: 6,
    padding: 10, marginVertical: 4, alignItems: 'center', borderWidth: 1,
  },
  keyKnown: { borderColor: '#4ecca3' },
  keyUnknown: { borderColor: '#333' },
  keyIdx: { color: '#666', fontSize: 10 },
  keyVal: { color: '#fff', fontSize: 18, fontWeight: 'bold', fontFamily: 'monospace' },
  fullKeyBox: { backgroundColor: '#4ecca322', borderRadius: 8, padding: 14, marginTop: 12, borderWidth: 1, borderColor: '#4ecca3' },
  fullKeyLabel: { color: '#4ecca3', fontSize: 12, marginBottom: 4 },
  fullKey: { color: '#fff', fontSize: 16, fontFamily: 'monospace', fontWeight: 'bold' },
  note: { color: '#a0a0c0', fontSize: 11, marginTop: 6 },
  help: { color: '#666', fontSize: 11, marginTop: 20, lineHeight: 16 },
});