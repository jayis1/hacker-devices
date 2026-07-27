// src/screens/TopologyScreen.tsx — PTP topology view
//
// Displays the discovered PTP network topology: grandmaster, transparent
// clocks, and ordinary/slave clocks with their attributes.
//
// Author: jayis1
// License: GPL-2.0

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, FlatList } from 'react-native';
import { useBle } from '../ble/BleManager';
import { PtpNode, EVT } from '../types';

export default function TopologyScreen() {
  const { connected, onEvent } = useBle();
  const [nodes, setNodes] = useState<PtpNode[]>([]);

  useEffect(() => {
    if (!connected) return;
    // Listen for topology events (parsed from captured Announce frames)
    const unsub = onEvent((frame) => {
      if (frame.opcode === EVT.FRAME_CAPTURED) {
        // Parse the frame to extract PTP node info
        const payload = frame.payload;
        if (payload.length < 10) return;
        const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
        const ts = Number(dv.getBigInt64(0, true));
        const frameLen = (payload[8] << 8) | payload[9];
        if (frameLen < 14) return;
        // Extract source MAC from frame bytes (offset 6 in Ethernet)
        const frameData = payload.subarray(10, 10 + Math.min(frameLen, 128));
        if (frameData.length < 14) return;
        const srcMac = Array.from(frameData.subarray(6, 12))
          .map((b) => b.toString(16).padStart(2, '0')).join(':');
        // PTP header starts at offset 14 (or 18 with VLAN)
        const ptpOff = frameData[12] === 0x81 && frameData[13] === 0x00 ? 18 : 14;
        if (frameData.length < ptpOff + 34) return;
        const msgType = frameData[ptpOff] & 0x0F;
        if (msgType !== 0x0B) return;  // Announce
        // Extract clockIdentity (bytes 20-27 of PTP header)
        const clockId = Array.from(frameData.subarray(ptpOff + 20, ptpOff + 28))
          .map((b) => b.toString(16).padStart(2, '0')).join(':');
        const priority1 = frameData[ptpOff + 56];
        const clockClass = frameData[ptpOff + 57];
        const node: PtpNode = {
          clockIdentity: clockId,
          portNumber: 1,
          isGrandmaster: clockClass < 128,
          clockClass,
          clockAccuracy: frameData[ptpOff + 58],
          priority1,
          priority2: frameData[ptpOff + 61],
          offsetFromMasterNs: 0,
          lastSeenMs: Date.now(),
        };
        setNodes((prev) => {
          const filtered = prev.filter((n) => n.clockIdentity !== clockId);
          return [node, ...filtered].slice(0, 32);
        });
      }
    });
    return unsub;
  }, [connected, onEvent]);

  const renderNode = ({ item }: { item: PtpNode }) => (
    <View style={[styles.nodeCard, item.isGrandmaster && styles.gmCard]}>
      <View style={styles.nodeHeader}>
        <Text style={styles.nodeIcon}>{item.isGrandmaster ? '👑' : '🕐'}</Text>
        <Text style={styles.nodeType}>
          {item.isGrandmaster ? 'GRANDMASTER' : 'SLAVE/ORDINARY'}
        </Text>
      </View>
      <Text style={styles.nodeLabel}>Clock ID</Text>
      <Text style={styles.nodeValue}>{item.clockIdentity}</Text>
      <View style={styles.nodeGrid}>
        <View style={styles.gridItem}>
          <Text style={styles.gridLabel}>Priority1</Text>
          <Text style={styles.gridValue}>{item.priority1}</Text>
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.gridLabel}>Class</Text>
          <Text style={styles.gridValue}>{item.clockClass}</Text>
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.gridLabel}>Accuracy</Text>
          <Text style={styles.gridValue}>0x{item.clockAccuracy.toString(16)}</Text>
        </View>
        <View style={styles.gridItem}>
          <Text style={styles.gridLabel}>Priority2</Text>
          <Text style={styles.gridValue}>{item.priority2}</Text>
        </View>
      </View>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>PTP Topology</Text>
      <Text style={styles.subtitle}>
        {nodes.length > 0
          ? `${nodes.length} clock${nodes.length > 1 ? 's' : ''} discovered`
          : 'No PTP clocks discovered yet. Start capture to map the topology.'}
      </Text>
      <FlatList
        data={nodes}
        renderItem={renderNode}
        keyExtractor={(item) => item.clockIdentity}
        contentContainerStyle={{ padding: 16, paddingBottom: 40 }}
        style={{ flex: 1 }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a0a', padding: 16 },
  title: { fontSize: 20, fontWeight: 'bold', color: '#00ff88' },
  subtitle: { fontSize: 12, color: '#666', marginTop: 4, marginBottom: 16 },
  nodeCard: {
    backgroundColor: '#1a1a1a', borderRadius: 10, padding: 16, marginBottom: 12,
    borderWidth: 1, borderColor: '#333',
  },
  gmCard: { borderColor: '#00ff88', borderWidth: 2 },
  nodeHeader: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  nodeIcon: { fontSize: 20, marginRight: 8 },
  nodeType: { fontSize: 14, fontWeight: '600', color: '#00ff88' },
  nodeLabel: { fontSize: 10, color: '#666', marginTop: 8 },
  nodeValue: { fontSize: 12, color: '#ccc', fontFamily: 'monospace' },
  nodeGrid: { flexDirection: 'row', marginTop: 12, justifyContent: 'space-between' },
  gridItem: { alignItems: 'center' },
  gridLabel: { fontSize: 9, color: '#666' },
  gridValue: { fontSize: 14, color: '#ccc', fontWeight: '600', marginTop: 2 },
});

// Author: jayis1
// License: GPL-2.0