// screens/TopologyScreen.js — discovered PLC station list + signal chart
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { useReaper } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import StationTable from '../components/StationTable';
import SignalChart from '../components/SignalChart';

export default function TopologyScreen() {
  const { stations, send } = useReaper();
  return (
    <View style={styles.container}>
      <StatusBar />
      <View style={styles.header}>
        <Text style={styles.h1}>PLC Topology</Text>
        <Text style={styles.sub}>{stations.length} stations discovered</Text>
      </View>

      <View style={styles.chartBox}>
        <Text style={styles.h2}>Signal Strength (SNR per station)</Text>
        <SignalChart stations={stations} />
      </View>

      <View style={styles.tableBox}>
        <Text style={styles.h2}>Stations</Text>
        <StationTable />
      </View>

      <TouchableOpacity style={styles.btn} onPress={() => send(0x0E /* topo refresh */)}>
        <Text style={styles.btnText}>Refresh</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 12 },
  header: { marginVertical: 12 },
  h1: { color: '#eee', fontSize: 20, fontWeight: 'bold' },
  h2: { color: '#00ffaa', fontSize: 13, fontFamily: 'monospace', marginBottom: 8 },
  sub: { color: '#888', fontSize: 12 },
  chartBox: { backgroundColor: '#111', padding: 10, borderRadius: 6, marginBottom: 12 },
  tableBox: { flex: 1, backgroundColor: '#111', padding: 10, borderRadius: 6 },
  btn: { backgroundColor: '#1a3a1a', padding: 12, borderRadius: 6, marginTop: 12, alignItems: 'center' },
  btnText: { color: '#00ffaa', fontSize: 14, fontWeight: 'bold' },
});