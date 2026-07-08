/*
 * tpm-phantom — app/screens/AnalyzeScreen.js
 * TPM transaction analysis and TPM register decode view.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useState, useMemo, useCallback } from 'react';
import { View, Text, TextInput, TouchableOpacity, FlatList, StyleSheet, ScrollView } from 'react-native';
import { tpmRegisterName, tpmLocality } from '../utils/protocol';

// TPM 2.0 register descriptions (TIS spec)
const TPM_REG_INFO = {
  'TPM_ACCESS': 'Locality access register — request/release locality, set/clear active',
  'TPM_STS': 'Status register — data available, expect, command ready',
  'TPM_HASH_START': 'Hash start register (PC Client-specific)',
  'TPM_DATA_FIFO': 'Data FIFO — read/write TPM command/response data',
  'TPM_INTERFACE_ID': 'Interface capabilities — supported bus widths, version',
  'TPM_DID_VID': 'Device ID / Vendor ID',
  'TPM_RID': 'Revision ID',
  'TPM_XFER_SIZE': 'Max transfer size supported',
};

// TPM ACCESS register bit decode
function decodeAccessReg(value) {
  const bits = [];
  if (value & 0x01) bits.push('ACTIVE_LOC0');
  if (value & 0x02) bits.push('ACTIVE_LOC1');
  if (value & 0x04) bits.push('ACTIVE_LOC2');
  if (value & 0x08) bits.push('ACTIVE_LOC3');
  if (value & 0x10) bits.push('ACTIVE_LOC4');
  if (value & 0x20) bits.push('PENDING_REQUEST');
  if (value & 0x40) bits.push('SEIZE');
  if (value & 0x80) bits.push('REQUEST_USE');
  return bits.length ? bits.join(', ') : '(none)';
}

function decodeStsReg(value) {
  const bits = [];
  if (value & 0x01) bits.push('DATA_AVAIL');
  if (value & 0x02) bits.push('EXPECT');
  if (value & 0x04) bits.push('SELF_TEST_DONE');
  if (value & 0x08) bits.push('VALID');
  if (value & 0x10) bits.push('READY');
  if (value & 0x20) bits.push('CMD_READY');
  if (value & 0x40) bits.push('BURST_COUNT');
  if (value & 0x80) bits.push('GO_BITS');
  return bits.length ? bits.join(', ') : '(none)';
}

export default function AnalyzeScreen({ transactions }) {
  const [searchAddr, setSearchAddr] = useState('');
  const [selectedTx, setSelectedTx] = useState(null);

  const filtered = useMemo(() => {
    if (!searchAddr) return transactions;
    const s = searchAddr.toLowerCase();
    return transactions.filter(t => {
      const regName = tpmRegisterName(t.address).toLowerCase();
      return regName.includes(s) || t.address.toString(16).includes(s);
    });
  }, [transactions, searchAddr]);

  // TPM register access frequency analysis
  const regStats = useMemo(() => {
    const map = {};
    transactions.forEach(t => {
      const name = tpmRegisterName(t.address);
      if (!map[name]) map[name] = { reads: 0, writes: 0, total: 0 };
      if (t.direction === 'READ' || t.command === 0x83) {
        map[name].reads++;
      } else {
        map[name].writes++;
      }
      map[name].total++;
    });
    return Object.entries(map).sort((a, b) => b[1].total - a[1].total);
  }, [transactions]);

  const renderItem = useCallback(({ item, index }) => {
    const regName = tpmRegisterName(item.address);
    const isRead = item.direction === 'READ' || item.command === 0x83;
    return (
      <TouchableOpacity
        style={[styles.row, index % 2 === 0 ? styles.rowEven : styles.rowOdd]}
        onPress={() => setSelectedTx(item)}>
        <Text style={isRead ? styles.read : styles.write}>{isRead ? 'R' : 'W'}</Text>
        <Text style={styles.reg}>{regName}</Text>
        <Text style={styles.addr}>0x{item.address.toString(16).toUpperCase()}</Text>
        <Text style={styles.data}>{item.bus === 'SPI'
          ? `${item.data.length}B`
          : `0x${item.data.toString(16).padStart(2, '0')}`}</Text>
      </TouchableOpacity>
    );
  }, []);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>TPM Register Access Frequency</Text>
        {regStats.length === 0 ? (
          <Text style={styles.empty}>No transactions captured yet</Text>
        ) : (
          regStats.map(([name, stats]) => (
            <View key={name} style={styles.regStatRow}>
              <Text style={styles.regStatName}>{name}</Text>
              <Text style={styles.regStatVal}>R:{stats.reads} W:{stats.writes}</Text>
              <View style={[styles.regBar, { width: Math.min(stats.total * 5, 200) }]} />
            </View>
          ))
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Transaction Search</Text>
        <TextInput
          style={styles.searchInput}
          value={searchAddr}
          onChangeText={setSearchAddr}
          placeholder="Filter by register name or address..."
          placeholderTextColor="#555"
        />
        <View style={styles.transactionList}>
          {filtered.slice(0, 100).map((item, i) => {
            const regName = tpmRegisterName(item.address);
            const isRead = item.direction === 'READ' || item.command === 0x83;
            return (
              <TouchableOpacity
                key={i}
                style={[styles.row, i % 2 === 0 ? styles.rowEven : styles.rowOdd]}
                onPress={() => setSelectedTx(item)}>
                <Text style={isRead ? styles.read : styles.write}>{isRead ? 'R' : 'W'}</Text>
                <Text style={styles.reg}>{regName}</Text>
                <Text style={styles.addr}>0x{item.address.toString(16).toUpperCase()}</Text>
                <Text style={styles.data}>{item.bus === 'SPI'
                  ? `${item.data.length}B`
                  : `0x${item.data.toString(16).padStart(2, '0')}`}</Text>
              </TouchableOpacity>
            );
          })}
        </View>
      </View>

      {selectedTx && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Transaction Detail</Text>
          <DetailRow label="Bus" value={selectedTx.bus} />
          <DetailRow label="Direction" value={selectedTx.direction || (selectedTx.command === 0x83 ? 'READ' : 'WRITE')} />
          <DetailRow label="Register" value={tpmRegisterName(selectedTx.address)} />
          <DetailRow label="Address" value={`0x${selectedTx.address.toString(16).toUpperCase()}`} />
          <DetailRow label="Locality" value={`L${tpmLocality(selectedTx.address)}`} />
          <DetailRow label="Data" value={selectedTx.bus === 'SPI'
            ? selectedTx.data.map(b => b.toString(16).padStart(2, '0')).join(' ')
            : `0x${selectedTx.data.toString(16).padStart(2, '0')}`} />
          <DetailRow label="Timestamp" value={`${(selectedTx.timestamp / 1000).toFixed(3)}s`} />

          {tpmRegisterName(selectedTx.address) === 'TPM_ACCESS' && (
            <View style={styles.decodeBox}>
              <Text style={styles.decodeTitle}>TPM_ACCESS decode:</Text>
              <Text style={styles.decodeText}>{decodeAccessReg(selectedTx.data || 0)}</Text>
            </View>
          )}
          {tpmRegisterName(selectedTx.address) === 'TPM_STS' && (
            <View style={styles.decodeBox}>
              <Text style={styles.decodeTitle}>TPM_STS decode:</Text>
              <Text style={styles.decodeText}>{decodeStsReg(selectedTx.data || 0)}</Text>
            </View>
          )}

          {TPM_REG_INFO[tpmRegisterName(selectedTx.address)] && (
            <View style={styles.infoBox}>
              <Text style={styles.infoText}>
                {TPM_REG_INFO[tpmRegisterName(selectedTx.address)]}
              </Text>
            </View>
          )}
        </View>
      )}
    </ScrollView>
  );
}

function DetailRow({ label, value }) {
  return (
    <View style={styles.detailRow}>
      <Text style={styles.detailLabel}>{label}:</Text>
      <Text style={styles.detailValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d1a' },
  card: {
    backgroundColor: '#1a1a2e',
    margin: 8,
    padding: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  cardTitle: {
    color: '#8888ff',
    fontSize: 15,
    fontWeight: 'bold',
    marginBottom: 8,
    fontFamily: 'monospace',
  },
  empty: { color: '#555', fontSize: 12, fontFamily: 'monospace', fontStyle: 'italic' },
  regStatRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  regStatName: { color: '#ccffcc', fontSize: 11, fontFamily: 'monospace', width: 120 },
  regStatVal: { color: '#ffdd44', fontSize: 11, fontFamily: 'monospace', width: 80 },
  regBar: { height: 8, backgroundColor: '#0a5', borderRadius: 4 },
  searchInput: {
    backgroundColor: '#111120',
    color: '#00ff88',
    fontFamily: 'monospace',
    fontSize: 13,
    padding: 8,
    borderRadius: 4,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  transactionList: { maxHeight: 300 },
  row: {
    flexDirection: 'row',
    paddingVertical: 4,
    paddingHorizontal: 6,
    borderBottomWidth: 0.5,
    borderBottomColor: '#222',
  },
  rowEven: { backgroundColor: '#111120' },
  rowOdd: { backgroundColor: '#0d0d1a' },
  read: { color: '#00ff88', fontSize: 11, fontFamily: 'monospace', width: 16, fontWeight: 'bold' },
  write: { color: '#ff6644', fontSize: 11, fontFamily: 'monospace', width: 16, fontWeight: 'bold' },
  reg: { color: '#ccccff', fontSize: 10, fontFamily: 'monospace', width: 100 },
  addr: { color: '#aaaaff', fontSize: 10, fontFamily: 'monospace', width: 80 },
  data: { color: '#ffdd44', fontSize: 10, fontFamily: 'monospace', flex: 1 },
  detailRow: { flexDirection: 'row', marginVertical: 3 },
  detailLabel: { color: '#888', fontSize: 12, fontFamily: 'monospace', width: 90 },
  detailValue: { color: '#00ff88', fontSize: 12, fontFamily: 'monospace', fontWeight: 'bold' },
  decodeBox: {
    marginTop: 8,
    padding: 8,
    backgroundColor: '#0a0a2a',
    borderRadius: 4,
    borderWidth: 1,
    borderColor: '#4488ff',
  },
  decodeTitle: { color: '#4488ff', fontSize: 11, fontFamily: 'monospace', fontWeight: 'bold' },
  decodeText: { color: '#aaccff', fontSize: 11, fontFamily: 'monospace', marginTop: 4 },
  infoBox: {
    marginTop: 8,
    padding: 8,
    backgroundColor: '#0a2a0a',
    borderRadius: 4,
  },
  infoText: { color: '#88cc88', fontSize: 10, fontFamily: 'monospace' },
});