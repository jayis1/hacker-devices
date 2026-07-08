/*
 * tpm-phantom — app/screens/CaptureScreen.js
 * Real-time capture view showing live TPM transactions.
 *
 * Author: jayis1
 * License: MIT
 */

'use strict';

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import TransactionLog from '../components/TransactionLog';
import {
  CMD_START_CAP, CMD_STOP_CAP, CMD_SET_FILTER,
  RSP_TRANSACTION, MODE_LPC, MODE_SPI,
  decodeTransaction, cmdStartCapture, cmdSetFilter,
} from '../utils/protocol';

const MAX_TRANSACTIONS = 1000;

export default function CaptureScreen({ ble, status, onFrame, onStatusUpdate }) {
  const [transactions, setTransactions] = useState([]);
  const [filterTpmOnly, setFilterTpmOnly] = useState(false);
  const [filterAddr, setFilterAddr] = useState('');

  // Register for transaction frames
  useEffect(() => {
    const handler = (cmd, payload) => {
      if (cmd === RSP_TRANSACTION) {
        const tx = decodeTransaction(payload);
        if (filterTpmOnly && !tx.isTpm) return;
        setTransactions(prev => [tx, ...prev].slice(0, MAX_TRANSACTIONS));
      }
    };
    ble.onFrame = handler;
    return () => { ble.onFrame = null; };
  }, [ble, filterTpmOnly]);

  const startCapture = useCallback((mode) => {
    setTransactions([]);
    ble.send(CMD_START_CAP, cmdStartCapture(mode, 0));
    onStatusUpdate({ ...status, capturing: true, mode });
  }, [ble, status, onStatusUpdate]);

  const stopCapture = useCallback(() => {
    ble.send(CMD_STOP_CAP);
    onStatusUpdate({ ...status, capturing: false });
  }, [ble, status, onStatusUpdate]);

  const applyFilter = useCallback(() => {
    if (filterAddr) {
      const addr = parseInt(filterAddr, 16);
      const mask = 0xFFFFFF;
      ble.send(CMD_SET_FILTER, cmdSetFilter(mask, addr));
    } else {
      ble.send(CMD_SET_FILTER, cmdSetFilter(0, 0));
    }
  }, [ble, filterAddr]);

  const clearLog = useCallback(() => {
    setTransactions([]);
  }, []);

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        <TouchableOpacity
          style={[styles.btn, status.mode === MODE_LPC && status.capturing ? styles.btnActive : styles.btnStart]}
          disabled={status.capturing}
          onPress={() => startCapture(MODE_LPC)}>
          <Text style={styles.btnText}>LPC</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.btn, status.mode === MODE_SPI && status.capturing ? styles.btnActive : styles.btnStart]}
          disabled={status.capturing}
          onPress={() => startCapture(MODE_SPI)}>
          <Text style={styles.btnText}>SPI</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.btn, styles.btnStop]}
          disabled={!status.capturing}
          onPress={stopCapture}>
          <Text style={styles.btnText}>STOP</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.btn, styles.btnClear]}
          onPress={clearLog}>
          <Text style={styles.btnText}>CLEAR</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.filterRow}>
        <TouchableOpacity
          style={[styles.filterBtn, filterTpmOnly ? styles.filterActive : null]}
          onPress={() => setFilterTpmOnly(!filterTpmOnly)}>
          <Text style={styles.filterBtnText}>TPM ONLY {filterTpmOnly ? '✓' : ''}</Text>
        </TouchableOpacity>
        <Text style={styles.filterLabel}>Addr Filter:</Text>
        <Text style={styles.filterInput} onPress={() => {
          // In a full app, this opens a modal text input
          const addr = prompt('Enter hex address filter (e.g. FED40000):', filterAddr || '');
          if (addr !== null) { setFilterAddr(addr); }
        }}>{filterAddr || 'none'}</Text>
        <TouchableOpacity style={styles.applyBtn} onPress={applyFilter}>
          <Text style={styles.btnText}>Apply</Text>
        </TouchableOpacity>
      </View>

      <TransactionLog transactions={transactions} maxItems={MAX_TRANSACTIONS} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d0d1a' },
  toolbar: {
    flexDirection: 'row',
    padding: 8,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  btn: {
    flex: 1,
    padding: 10,
    borderRadius: 6,
    alignItems: 'center',
    marginHorizontal: 3,
  },
  btnStart: { backgroundColor: '#0a5' },
  btnStop: { backgroundColor: '#a33' },
  btnClear: { backgroundColor: '#444' },
  btnActive: { backgroundColor: '#0a5', borderWidth: 2, borderColor: '#0ff' },
  btnText: { color: '#fff', fontSize: 12, fontFamily: 'monospace', fontWeight: 'bold' },
  filterRow: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#151525',
    borderBottomWidth: 1,
    borderBottomColor: '#222',
  },
  filterBtn: {
    padding: 6,
    backgroundColor: '#333',
    borderRadius: 4,
    marginRight: 8,
  },
  filterActive: { backgroundColor: '#5a0' },
  filterBtnText: { color: '#fff', fontSize: 11, fontFamily: 'monospace' },
  filterLabel: { color: '#888', fontSize: 11, fontFamily: 'monospace', marginRight: 4 },
  filterInput: {
    color: '#aaffff',
    fontSize: 11,
    fontFamily: 'monospace',
    backgroundColor: '#222',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 4,
    flex: 1,
    marginRight: 6,
  },
  applyBtn: {
    backgroundColor: '#05a',
    padding: 6,
    borderRadius: 4,
  },
});