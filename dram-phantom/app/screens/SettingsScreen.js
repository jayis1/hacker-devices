/**
 * SettingsScreen.js — device info, connection, battery, firmware, self-test
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useCallback } from 'react';
import { View, Text, StyleSheet, Button, Alert, ScrollView } from 'react-native';
import { useDevice } from '../components/DeviceContext';

export default function SettingsScreen() {
  const { connected, transport, status, connectBLE, connectUSB, disconnect, refreshStatus } = useDevice();
  const [busy, setBusy] = useState(false);

  const doConnectBLE = useCallback(async () => {
    setBusy(true);
    try {
      await connectBLE();
      await refreshStatus();
    } catch (e) {
      Alert.alert('BLE connect failed', e.message);
    } finally {
      setBusy(false);
    }
  }, [connectBLE, refreshStatus]);

  const doConnectUSB = useCallback(async () => {
    setBusy(true);
    try {
      await connectUSB();
      await refreshStatus();
    } catch (e) {
      Alert.alert('USB connect failed', e.message);
    } finally {
      setBusy(false);
    }
  }, [connectUSB, refreshStatus]);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Settings</Text>
        <Text style={styles.status}>{connected ? `${transport} · connected` : 'disconnected'}</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.controls}>
          <Button title={busy ? 'Connecting...' : 'Connect BLE'} onPress={doConnectBLE} disabled={busy} />
          <Button title="Connect USB" onPress={doConnectUSB} disabled={busy} />
          <Button title="Disconnect" color="#d73a49" onPress={disconnect} disabled={!connected} />
        </View>
        <Button title="Refresh Status" onPress={refreshStatus} />
      </View>

      {status && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Device Status</Text>
          <Text style={styles.info}>Mode: {['IDLE','SNOOP','ROWHAMMER','WARMBOOT','COVERT'][status.mode]}</Text>
          <Text style={styles.info}>Armed: {status.armed ? 'YES' : 'no'}</Text>
          <Text style={styles.info}>FPGA status: 0x{status.fpga.toString(16).padStart(8, '0')}</Text>
          <Text style={styles.info}>Battery: {status.batt_mv} mV</Text>
          <Text style={styles.info}>Arm token: {status.token}</Text>
          <Text style={styles.info}>DDR present: {status.fpga & 0x02 ? 'YES' : 'no'}</Text>
          <Text style={styles.info}>Refresh active: {status.fpga & 0x10 ? 'YES' : 'no'}</Text>
        </View>
      )}

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.info}>Device: DRAM-Phantom v1.0</Text>
        <Text style={styles.info}>Author: jayis1</Text>
        <Text style={styles.info}>License: GPL-2.0 / CERN-OHL-S v2</Text>
        <Text style={styles.info}>MCU: STM32G474CEU6 (Cortex-M4F, 170 MHz)</Text>
        <Text style={styles.info}>FPGA: Xilinx Artix-7 XC7A100T</Text>
        <Text style={styles.info}>BLE: Nordic nRF52840 (BT840F)</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Legal</Text>
        <Text style={styles.legal}>
          This device is for authorized security research only. Memory
          acquisition, Rowhammer injection, and covert-channel timing on
          systems you do not own or have not been authorized to assess may
          violate computer-fraud, data-protection, and physical-tampering
          laws. The author (jayis1) assumes no liability for misuse. Always
          obtain proper written authorization before deployment.
        </Text>
      </View>

      <Text style={styles.footer}>DRAM-Phantom v1.0 · jayis1 · 2026</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1117', padding: 12 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  title: { color: '#e6edf3', fontSize: 18, fontWeight: 'bold' },
  status: { color: '#7d8590', fontSize: 12 },
  section: { backgroundColor: '#161b22', borderRadius: 4, padding: 12, marginBottom: 12 },
  sectionTitle: { color: '#58a6ff', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  controls: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 8 },
  info: { color: '#e6edf3', fontSize: 12, fontFamily: 'monospace', marginBottom: 4 },
  legal: { color: '#ffa198', fontSize: 11, lineHeight: 16 },
  footer: { color: '#484f58', fontSize: 10, textAlign: 'center', marginTop: 8, marginBottom: 16 },
});