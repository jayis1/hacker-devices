// src/components/FrameLogTable.tsx — Frame log table component
//
// Author: jayis1
// License: GPL-2.0

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { CapturedFrame } from '../types';

const MSG_TYPE_NAMES: Record<number, string> = {
  0x0: 'Sync', 0x1: 'DelayReq', 0x2: 'PdelayReq', 0x3: 'PdelayResp',
  0x8: 'FollowUp', 0x9: 'DelayResp', 0xA: 'PdelayRespFU',
  0xB: 'Announce', 0xC: 'Signal', 0xD: 'Mgmt',
};

interface FrameLogTableProps {
  frames: CapturedFrame[];
  maxRows?: number;
}

export default function FrameLogTable({ frames, maxRows = 50 }: FrameLogTableProps) {
  const shown = frames.slice(0, maxRows);
  return (
    <View style={styles.container}>
      <View style={styles.headerRow}>
        <Text style={[styles.cell, styles.cellType]}>Type</Text>
        <Text style={[styles.cell, styles.cellMac]}>Src MAC</Text>
        <Text style={[styles.cell, styles.cellLen]}>Len</Text>
        <Text style={[styles.cell, styles.cellCf]}>Correction</Text>
      </View>
      <ScrollView>
        {shown.map((f, i) => (
          <View key={i} style={[styles.row, i % 2 === 0 && styles.rowAlt]}>
            <Text style={[styles.cell, styles.cellType, styles.cellMono]}>
              {f.msgType >= 0 ? (MSG_TYPE_NAMES[f.msgType] || `0x${f.msgType.toString(16)}`) : '—'}
            </Text>
            <Text style={[styles.cell, styles.cellMac, styles.cellMono]}>{f.srcMac}</Text>
            <Text style={[styles.cell, styles.cellLen, styles.cellMono]}>{f.frameLen}</Text>
            <Text style={[styles.cell, styles.cellCf, styles.cellMono]}>
              {f.correctionField || '—'}
            </Text>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { backgroundColor: '#1a1a1a', borderRadius: 8, overflow: 'hidden' },
  headerRow: {
    flexDirection: 'row', backgroundColor: '#0a0a0a', paddingVertical: 8,
    borderBottomWidth: 1, borderBottomColor: '#333',
  },
  row: { flexDirection: 'row', paddingVertical: 6, borderBottomWidth: 1, borderBottomColor: '#111' },
  rowAlt: { backgroundColor: '#161616' },
  cell: { fontSize: 10, color: '#999', paddingHorizontal: 6 },
  cellMono: { fontFamily: 'monospace' },
  cellType: { width: 70, color: '#00ff88' },
  cellMac: { flex: 1 },
  cellLen: { width: 50, textAlign: 'right' },
  cellCf: { width: 100 },
});

// Author: jayis1
// License: GPL-2.0