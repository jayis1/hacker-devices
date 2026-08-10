/**
 * PDMessage.js — Renders a single decoded PD frame
 *
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const SOF_NAMES = ['SOP', "SOP'", "SOP''"];

const CTRL_NAMES = {
  0x01: 'GoodCRC', 0x02: 'GoToMin', 0x03: 'Accept', 0x04: 'Reject',
  0x05: 'Ping', 0x06: 'PS_RDY', 0x07: 'GetSrcCap', 0x08: 'GetSnkCap',
  0x09: 'DR_Swap', 0x0A: 'PR_Swap', 0x0B: 'VCONN_Swap', 0x0C: 'Wait',
  0x0D: 'SoftReset', 0x10: 'NotSupported', 0x17: 'GetSourceCap',
};

const DATA_NAMES = {
  0x01: 'Source Cap', 0x02: 'Request', 0x04: 'Sink Cap',
  0x0F: 'VDM', 0x1F: 'Src Cap Extended',
};

function getMsgName(msgType, numobj) {
  if (numobj > 0) {
    return DATA_NAMES[msgType] || `Data #${msgType}`;
  }
  return CTRL_NAMES[msgType] || `Control #${msgType}`;
}

export default function PDMessage({ frame, compact = false }) {
  if (!frame) return null;
  const sofName = SOF_NAMES[frame.sof] || `SOF${frame.sof}`;
  const msgName = getMsgName(frame.msg_type, frame.numobj);
  const roleStr = `${frame.power_role ? 'SRC' : 'SNK'}/${frame.data_role ? 'DFP' : 'UFP'}`;
  const hdrHex = `0x${frame.raw.toString(16).padStart(4, '0').toUpperCase()}`;

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.timestamp}>{frame.ts_ms}ms</Text>
        <Text style={styles.sof}>{sofName}</Text>
        <Text style={styles.role}>{roleStr}</Text>
      </View>
      <View style={styles.body}>
        <Text style={styles.msgName}>{msgName}</Text>
        {!compact && (
          <View style={styles.details}>
            <Text style={styles.detailText}>Header: {hdrHex}</Text>
            <Text style={styles.detailText}>MsgID: {frame.msg_id}</Text>
            <Text style={styles.detailText}>Objects: {frame.numobj}</Text>
            <Text style={styles.detailText}>Rev: {frame.spec_rev}</Text>
          </View>
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#16213e', marginHorizontal: 8, marginVertical: 4,
    borderRadius: 6, padding: 10, borderLeftWidth: 3, borderLeftColor: '#FF6600',
  },
  header: { flexDirection: 'row', marginBottom: 4 },
  timestamp: { color: '#888', fontSize: 11, marginRight: 8 },
  sof: { color: '#FF6600', fontSize: 11, fontWeight: 'bold', marginRight: 8 },
  role: { color: '#4CAF50', fontSize: 11 },
  body: { flexDirection: 'row', justifyContent: 'space-between' },
  msgName: { color: '#fff', fontSize: 14, fontWeight: '600' },
  details: { alignItems: 'flex-end' },
  detailText: { color: '#aaa', fontSize: 10 },
});

/* end of file — author: jayis1 */