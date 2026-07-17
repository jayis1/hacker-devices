// src/components/ConnectionBar.tsx — top bar showing connection + mode status.
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.

import React from 'react';
import { View, Text, Pressable, StyleSheet } from 'react-native';
import { useDevice } from '../state/DeviceContext';
import { MODES } from '../protocol';

export function ConnectionBar() {
  const { connected, connecting, activeMode, connect, disconnect, error } = useDevice();
  const modeInfo = MODES.find((m) => m.id === activeMode);

  return (
    <View style={styles.bar}>
      <View style={[styles.dot, { backgroundColor: connected ? '#2a9d8f' : '#e63946' }]} />
      <Text style={styles.statusText}>
        {connected ? 'Connected' : connecting ? 'Connecting…' : 'Disconnected'}
        {' · '}
        <Text style={{ color: modeInfo?.color ?? '#fff' }}>{modeInfo?.label ?? activeMode}</Text>
      </Text>
      <View style={{ flex: 1 }} />
      {error ? <Text style={styles.err}>{error}</Text> : null}
      <Pressable
        style={[styles.btn, { backgroundColor: connected ? '#555' : '#d62828' }]}
        onPress={connected ? disconnect : connect}
      >
        <Text style={styles.btnText}>{connected ? 'Disconnect' : 'Connect'}</Text>
      </Pressable>
    </View>
  );
}

const styles = StyleSheet.create({
  bar:        { flexDirection: 'row', alignItems: 'center', padding: 8, backgroundColor: '#111', borderBottomWidth: 1, borderColor: '#333' },
  dot:        { width: 10, height: 10, borderRadius: 5, marginRight: 8 },
  statusText: { color: '#eee', fontSize: 12 },
  err:        { color: '#e63946', fontSize: 11, marginRight: 8 },
  btn:        { paddingHorizontal: 12, paddingVertical: 6, borderRadius: 6 },
  btnText:    { color: 'white', fontWeight: '600', fontSize: 12 },
});