/*
 * DashboardScreen.js — CoilGrip status overview
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

import React, { useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Card, Title, Paragraph, Button, ActivityIndicator } from 'react-native-paper';
import { useDevice } from '../components/DeviceContext';

export default function DashboardScreen() {
  const { connected, connecting, telemetry, connect, disconnect, sendCommand, log } = useDevice();

  // Request periodic status from the device
  useEffect(() => {
    if (!connected) return;
    const id = setInterval(() => { sendCommand('status\n'); }, 2000);
    return () => clearInterval(id);
  }, [connected, sendCommand]);

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>CoilGrip v1.0</Title>
          <Paragraph style={styles.author}>Author: jayis1 · GPL-2.0</Paragraph>
          <Paragraph>Status: {connected ? 'Connected' : connecting ? 'Connecting…' : 'Disconnected'}</Paragraph>
          <Paragraph>Mode: {telemetry.mode}</Paragraph>
          <Paragraph>Coil temp: {telemetry.coilTempC} °C</Paragraph>
          <Paragraph>Current: {telemetry.currentA.toFixed(2)} A</Paragraph>
          <Paragraph>Power: {telemetry.powerW.toFixed(2)} W</Paragraph>
          <Paragraph>FOD unlock: {telemetry.fodUnlocked ? 'YES' : 'no'}</Paragraph>
          <Paragraph>Glitch armed: {telemetry.glitchArmed ? 'yes' : 'no'}</Paragraph>
          <Paragraph>Glitch fires: {telemetry.glitchFires}</Paragraph>
        </Card.Content>
        <Card.Actions>
          {!connected
            ? <Button mode="contained" onPress={connect} loading={connecting}>Connect BLE</Button>
            : <Button mode="outlined" onPress={disconnect}>Disconnect</Button>}
        </Card.Actions>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Mode</Title>
          <View style={styles.row}>
            <Button mode="text" onPress={() => sendCommand('mode idle\n')}>Idle</Button>
            <Button mode="text" onPress={() => sendCommand('mode sniffer\n')}>Sniffer</Button>
            <Button mode="text" onPress={() => sendCommand('mode mitm\n')}>MITM</Button>
          </View>
          <View style={styles.row}>
            <Button mode="text" onPress={() => sendCommand('mode profile\n')}>Profile</Button>
            <Button mode="text" onPress={() => sendCommand('mode glitch\n')}>Glitch</Button>
            <Button mode="text" onPress={() => sendCommand('mode exfil\n')}>Exfil</Button>
          </View>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Recent log</Title>
          {log.slice(-6).map((e, i) => (
            <Paragraph key={i} style={styles.logLine}>[{e.type}] {e.msg}</Paragraph>
          ))}
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 12, backgroundColor: '#0d1117' },
  card: { marginBottom: 12, backgroundColor: '#161b22' },
  author: { color: '#8b949e', fontStyle: 'italic' },
  row: { flexDirection: 'row', flexWrap: 'wrap', marginBottom: 4 },
  logLine: { fontFamily: 'monospace', color: '#58a6ff', fontSize: 11 },
});