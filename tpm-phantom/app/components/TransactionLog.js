/*
 * tpm-phantom — app/components/TransactionLog.js
 * Scrollable list of captured TPM transactions.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useRef, useEffect } from 'react';
import { View, Text, FlatList, StyleSheet } from 'react-native';
import { tpmRegisterName, tpmLocality } from '../utils/protocol';

export default function TransactionLog({ transactions, maxItems = 500 }) {
  const flatListRef = useRef(null);

  useEffect(() => {
    if (transactions.length > 0 && flatListRef.current) {
      flatListRef.current.scrollToOffset({ offset: 0, animated: false });
    }
  }, [transactions]);

  const renderItem = ({ item, index }) => {
    const isRead = item.direction === 'READ' || item.command === 0x83;
    const regName = tpmRegisterName(item.address);
    const loc = item.bus === 'LPC' ? '' : ` L${tpmLocality(item.address)}`;
    const dataStr = item.bus === 'SPI'
      ? item.data.map(b => b.toString(16).padStart(2, '0')).join(' ')
      : `0x${item.data.toString(16).padStart(2, '0')}`;

    return (
      <View style={[styles.row, index % 2 === 0 ? styles.rowEven : styles.rowOdd]}>
        <Text style={styles.idx}>{String(index).padStart(4, '0')}</Text>
        <Text style={[styles.bus, item.bus === 'LPC' ? styles.busLpc : styles.busSpi]}>
          {item.bus}
        </Text>
        <Text style={isRead ? styles.read : styles.write}>
          {isRead ? 'R' : 'W'}
        </Text>
        <Text style={styles.reg}>{regName}{loc}</Text>
        <Text style={styles.addr}>0x{item.address.toString(16).padStart(6, '0').toUpperCase()}</Text>
        <Text style={styles.data}>{dataStr}</Text>
        <Text style={styles.time}>{(item.timestamp / 1000).toFixed(3)}s</Text>
        {item.isTpm ? <Text style={styles.tpmFlag}> TPM</Text> : null}
      </View>
    );
  };

  const display = transactions.slice(0, maxItems);

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerText}>TPM Transaction Log — {transactions.length} entries</Text>
      </View>
      <FlatList
        ref={flatListRef}
        data={display}
        renderItem={renderItem}
        keyExtractor={(item, index) => `${item.bus}-${index}-${item.timestamp}`}
        inverted={false}
        removeClippedSubviews={true}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d1a' },
  header: {
    backgroundColor: '#1a1a2e',
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  headerText: { color: '#8888ff', fontSize: 13, fontFamily: 'monospace', fontWeight: 'bold' },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 3,
    paddingHorizontal: 8,
    borderBottomWidth: 0.5,
    borderBottomColor: '#222',
  },
  rowEven: { backgroundColor: '#111120' },
  rowOdd: { backgroundColor: '#0d0d1a' },
  idx: { color: '#555', fontSize: 10, fontFamily: 'monospace', width: 40 },
  bus: { fontSize: 10, fontFamily: 'monospace', fontWeight: 'bold', width: 36 },
  busLpc: { color: '#ffaa00' },
  busSpi: { color: '#00aaff' },
  read: { color: '#00ff88', fontSize: 11, fontFamily: 'monospace', width: 16, fontWeight: 'bold' },
  write: { color: '#ff6644', fontSize: 11, fontFamily: 'monospace', width: 16, fontWeight: 'bold' },
  reg: { color: '#ccccff', fontSize: 10, fontFamily: 'monospace', width: 120 },
  addr: { color: '#aaaaff', fontSize: 10, fontFamily: 'monospace', width: 80 },
  data: { color: '#ffdd44', fontSize: 10, fontFamily: 'monospace', flex: 1 },
  time: { color: '#666', fontSize: 9, fontFamily: 'monospace', width: 60 },
  tpmFlag: { color: '#ff00ff', fontSize: 9, fontFamily: 'monospace', fontWeight: 'bold' },
});