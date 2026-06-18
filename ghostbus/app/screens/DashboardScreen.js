/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * Companion App — Dashboard Screen
 *
 * Shows connection status, battery, target SoC info, and quick actions.
 *
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert } from 'react-native';

export default function DashboardScreen({ device, connected, battery, targetState,
                                          discovery, connect, sendCommand }) {
  const stateColors = {
    IDLE: '#888', DISCOVERY: '#00aaff', SWD_CONNECTED: '#00ff88',
    JTAG_CONNECTED: '#00ff88', FLASH_DUMP: '#ff00ff', LOCKED: '#ff3333'
  };

  const handleDiscover = () => {
    sendCommand(0x01, Buffer.from([3])); // discover with all 3 protocols
  };

  const handleHalt = () => sendCommand(0x04, Buffer.alloc(0));
  const handleResume = () => sendCommand(0x05, Buffer.alloc(0));
  const handleLock = () => {
    Alert.alert('Lock Device', 'Disconnect and lock GHOSTBUS?', [
      { text: 'Cancel' },
      { text: 'Lock', onPress: () => sendCommand(0x0D, Buffer.alloc(0)) }
    ]);
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>
        <View style={styles.row}>
          <Text style={styles.label}>BLE:</Text>
          <Text style={[styles.value, { color: connected ? '#00ff88' : '#ff3333' }]}>
            {connected ? 'CONNECTED' : 'DISCONNECTED'}
          </Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Device:</Text>
          <Text style={styles.value}>{device ? device.name || device.id : '—'}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Battery:</Text>
          <Text style={styles.value}>{battery}%</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>State:</Text>
          <Text style={[styles.value, { color: stateColors[targetState] || '#888' }]}>
            {targetState}
          </Text>
        </View>
        {!connected && (
          <TouchableOpacity style={styles.button} onPress={connect}>
            <Text style={styles.buttonText}>Connect to GHOSTBUS</Text>
          </TouchableOpacity>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Target Identification</Text>
        {discovery ? (
          <>
            <View style={styles.row}>
              <Text style={styles.label}>Protocol:</Text>
              <Text style={styles.value}>{discovery.protocol}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>IDCODE:</Text>
              <Text style={styles.value}>{discovery.idcode}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>VRef:</Text>
              <Text style={styles.value}>{discovery.vref_mv} mV</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>GND CH:</Text>
              <Text style={styles.value}>CH{discovery.gnd_ch}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>Confidence:</Text>
              <Text style={styles.value}>{discovery.confidence}%</Text>
            </View>
          </>
        ) : (
          <Text style={styles.muted}>No target discovered yet</Text>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Actions</Text>
        <TouchableOpacity style={styles.actionBtn} onPress={handleDiscover}>
          <Text style={styles.actionText}>🔍 Auto-Discover Debug Port</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn} onPress={handleHalt}
                          disabled={targetState !== 'SWD_CONNECTED'}>
          <Text style={styles.actionText}>⏸  Halt Core</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn} onPress={handleResume}
                          disabled={targetState !== 'SWD_CONNECTED'}>
          <Text style={styles.actionText}>▶  Resume Core</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionBtn, { borderColor: '#ff3333' }]}
                          onPress={handleLock}>
          <Text style={[styles.actionText, { color: '#ff3333' }]}>🔒 Lock & Disconnect</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1a', padding: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginBottom: 12,
          borderWidth: 1, borderColor: '#2a2a4e' },
  cardTitle: { color: '#00ff88', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  label: { color: '#888', fontSize: 14 },
  value: { color: '#e0e0e0', fontSize: 14, fontWeight: '500' },
  muted: { color: '#666', fontSize: 14, fontStyle: 'italic' },
  button: { backgroundColor: '#00ff88', borderRadius: 6, padding: 12,
            marginTop: 12, alignItems: 'center' },
  buttonText: { color: '#000', fontWeight: 'bold', fontSize: 14 },
  actionBtn: { borderWidth: 1, borderColor: '#00ff88', borderRadius: 6,
              padding: 12, marginBottom: 8, alignItems: 'center' },
  actionText: { color: '#00ff88', fontSize: 14, fontWeight: '500' },
});