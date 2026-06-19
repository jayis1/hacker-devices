/**
 * ScanProgressScreen.tsx — live scan progress + fault heatmap
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Dimensions } from 'react-native';
import { useBle, FAULT_LABELS } from '../services/BleContext';
import { FaultHeatmap } from '../components/FaultHeatmap';

const { width } = Dimensions.get('window');

export default function ScanProgressScreen({ navigation }: any) {
  const ble = useBle();
  const st = ble.scanStatus;

  const onStart = async () => { await ble.startScan(); };
  const onAbort = async () => { await ble.abortScan(); };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.statusCard}>
        <Text style={styles.statusTitle}>Scan Status</Text>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Shots:</Text>
          <Text style={styles.v}>{st?.shotsTotal ?? 0}</Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Faults:</Text>
          <Text style={[styles.v, { color: '#e94560' }]}>{st?.faultsTotal ?? 0}</Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>Target clock:</Text>
          <Text style={styles.v}>{((st?.targetClkHz ?? 0) / 1000).toFixed(2)} MHz</Text>
        </View>
        <View style={styles.kvRow}>
          <Text style={styles.k}>State:</Text>
          <Text style={styles.v}>
            {st ? (st.code === 0xA11C ? 'KEY RECOVERED' :
                   st.code === 0xF1F15F ? 'Finished' :
                   st.code === 0xDEAD ? 'SAFETY TRIP' :
                   st.code === 0xFFFFFFFF ? 'Running' : 'Idle') : 'Idle'}
          </Text>
        </View>
      </View>

      <Text style={styles.section}>Fault Heatmap</Text>
      <FaultHeatmap grid={ble.faultGrid} size={width - 32} />

      <View style={styles.buttonRow}>
        <TouchableOpacity style={[styles.button, { borderColor: '#4ecca3' }]} onPress={onStart}>
          <Text style={[styles.buttonText, { color: '#4ecca3' }]}>Start Scan</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.button, { borderColor: '#e94560' }]} onPress={onAbort}>
          <Text style={styles.buttonText}>Abort</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity
        style={[styles.button, { marginTop: 8 }]}
        onPress={() => navigation.navigate('DFA')}
      >
        <Text style={styles.buttonText}>View DFA Results →</Text>
      </TouchableOpacity>

      <Text style={styles.legend}>
        Legend: {Object.entries(FAULT_LABELS).map(([k, v]) => `${k}:${v}`).join('  ')}
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  statusCard: { backgroundColor: '#16213e', borderRadius: 10, padding: 16, marginBottom: 12 },
  statusTitle: { color: '#e94560', fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  kvRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 2 },
  k: { color: '#a0a0c0', fontSize: 14 },
  v: { color: '#fff', fontSize: 14, fontWeight: '600' },
  section: { color: '#e94560', fontSize: 14, fontWeight: '700', marginTop: 8, marginBottom: 6 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 12 },
  button: {
    flex: 1, backgroundColor: '#16213e', padding: 14, borderRadius: 8,
    alignItems: 'center', marginHorizontal: 4, borderWidth: 1, borderColor: '#e94560',
  },
  buttonText: { color: '#e94560', fontSize: 16, fontWeight: '600' },
  legend: { color: '#666', fontSize: 10, marginTop: 16, flexWrap: 'wrap' },
});