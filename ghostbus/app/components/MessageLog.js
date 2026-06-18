/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — MessageLog Component
 *
 * Renders a scrollable list of session log entries.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, FlatList } from 'react-native';

export function MessageLog({ logs }) {
  const renderItem = ({ item }) => (
    <View style={styles.row}>
      <Text style={styles.ts}>[{item.ts}]</Text>
      <Text style={[styles.msg, item.level === 'error' && styles.msgError,
                    item.level === 'warn' && styles.msgWarn,
                    item.level === 'success' && styles.msgSuccess]}>
        {item.msg}
      </Text>
    </View>
  );

  return (
    <FlatList data={logs} renderItem={renderItem}
              keyExtractor={(item, i) => i.toString()}
              style={styles.container} />
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#000', borderRadius: 4,
               padding: 6 },
  row: { flexDirection: 'row', paddingVertical: 2 },
  ts: { color: '#555', fontSize: 11, fontFamily: 'monospace' },
  msg: { color: '#aaa', fontSize: 12, marginLeft: 6, flex: 1,
         fontFamily: 'monospace' },
  msgError: { color: '#ff5555' },
  msgWarn: { color: '#ffaa00' },
  msgSuccess: { color: '#00ff88' },
});