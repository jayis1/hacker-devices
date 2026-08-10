/**
 * SnifferScreen.js — Passive PD frame capture display
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useCallback, useEffect } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity, Switch,
} from 'react-native';
import PDMessage from '../components/PDMessage';
import { CMD, RSP, EmberTapConnection } from '../utils/protocol';

const CTRL_NAMES = {
  0x01: 'GoodCRC', 0x02: 'GoToMin', 0x03: 'Accept', 0x04: 'Reject',
  0x05: 'Ping', 0x06: 'PS_RDY', 0x07: 'GetSrcCap', 0x08: 'GetSnkCap',
  0x09: 'DR_Swap', 0x0A: 'PR_Swap', 0x0B: 'VCONN_Swap', 0x0C: 'Wait',
  0x0D: 'SoftReset', 0x10: 'NotSupported',
};

function msgTypeName(type, isData) {
  if (isData) {
    switch (type) {
      case 1: return 'SrcCap';
      case 2: return 'Request';
      case 4: return 'SnkCap';
      case 0xF: return 'VDM';
      default: return `Data#${type}`;
    }
  }
  return CTRL_NAMES[type] || `Ctrl#${type}`;
}

export default function SnifferScreen() {
  const [frames, setFrames] = useState([]);
  const [sniffing, setSniffing] = useState(false);
  const [conn, setConn] = useState(null);

  const handlePacket = useCallback((pkt) => {
    if (pkt.cmd === RSP.PD_FRAME) {
      const { decodePDFrame } = require('../utils/protocol');
      const f = decodePDFrame(pkt.payload);
      if (f) {
        setFrames(prev => [{
          key: `${f.ts_ms}-${prev.length}`,
          ...f,
        }, ...prev].slice(0, 500));
      }
    }
  }, []);

  const toggleSniff = async () => {
    if (!conn) return;
    await conn.send(sniffing ? CMD.SNIFF_OFF : CMD.SNIFF_ON);
    setSniffing(!sniffing);
  };

  const renderItem = ({ item }) => (
    <PDMessage frame={item} />
  );

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        <Text style={styles.toolbarTitle}>PD Sniffer</Text>
        <Switch
          value={sniffing}
          onValueChange={toggleSniff}
          trackColor={{ false: '#333', true: '#FF6600' }}
        />
        <TouchableOpacity onPress={() => setFrames([])}>
          <Text style={styles.clearBtn}>Clear</Text>
        </TouchableOpacity>
      </View>

      <FlatList
        data={frames}
        renderItem={renderItem}
        keyExtractor={item => item.key}
        ListEmptyComponent={
          <Text style={styles.empty}>
            No frames captured. Enable sniffing to begin.
          </Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e' },
  toolbar: {
    flexDirection: 'row', alignItems: 'center', padding: 12,
    borderBottomWidth: 1, borderBottomColor: '#222',
  },
  toolbarTitle: { color: '#FF6600', fontSize: 18, fontWeight: 'bold', flex: 1 },
  clearBtn: { color: '#FF6600', marginLeft: 16, fontSize: 14 },
  empty: { color: '#666', textAlign: 'center', marginTop: 40, fontSize: 14 },
});

/* end of file — author: jayis1 */