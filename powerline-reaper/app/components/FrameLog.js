// components/FrameLog.js — live captured PLC frame list
// Author: jayis1
// License: MIT

import React from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';
import { useReaper } from '../utils/protocol';

const OPC = { 0x01:'Beacon', 0x02:'Discover', 0x03:'AssocReq', 0x04:'AssocCnf',
              0x07:'Disassoc', 0x0C:'SetKey', 0x0D:'GetNMK' };

export default function FrameLog({ filter = null }) {
  const { frames } = useReaper();
  const shown = filter ? frames.filter((f) => f.opcode === filter) : frames;
  return (
    <FlatList
      data={shown}
      keyExtractor={(item, idx) => idx.toString()}
      renderItem={({ item }) => (
        <View style={styles.row}>
          <Text style={styles.ts}>{item.ts}</Text>
          <Text style={styles.opc}>{OPC[item.opcode] || ('0x'+item.opcode.toString(16))}</Text>
          <Text style={styles.mac}>{item.src}</Text>
          <Text style={styles.arrow}>→</Text>
          <Text style={styles.mac}>{item.dst}</Text>
          <Text style={styles.snr}>{item.snr}dB</Text>
        </View>
      )}
      ListEmptyComponent={<Text style={styles.empty}>No frames captured</Text>}
    />
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', padding: 6, borderBottomWidth: 0.5, borderBottomColor: '#222', alignItems: 'center' },
  ts: { color: '#888', fontSize: 10, fontFamily: 'monospace', width: 60 },
  opc: { color: '#00ffaa', fontSize: 11, fontFamily: 'monospace', width: 70 },
  mac: { color: '#ccc', fontSize: 10, fontFamily: 'monospace', width: 120 },
  arrow: { color: '#666', marginHorizontal: 4 },
  snr: { color: '#ffaa00', fontSize: 10, fontFamily: 'monospace', marginLeft: 'auto' },
  empty: { color: '#666', textAlign: 'center', padding: 20 },
});