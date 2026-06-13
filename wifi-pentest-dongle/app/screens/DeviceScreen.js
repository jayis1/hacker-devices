/**
 * WFP-100 Device Screen — Connection and device status
 *
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { buildFrame, CMD_CONNECT, CMD_DISCONNECT, CMD_GET_STATUS, CMD_SET_MODE, parseFrame, RESP_STATUS, MODE_MONITOR, MODE_IDLE } from '../utils/protocol';
import StatusCard from '../components/StatusCard';

export default function DeviceScreen({ navigation }) {
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState({
    mode: MODE_IDLE,
    channel: 6,
    band: 0,
    monitorEnabled: false,
    capturing: false,
    injecting: false,
    packetsCaptured: 0,
    packetsInjected: 0,
    batteryMv: 0,
    temperature: 0,
  });

  useEffect(() => {
    if (connected) {
      const interval = setInterval(() => {
        sendCommand(CMD_GET_STATUS, null);
      }, 2000);
      return () => clearInterval(interval);
    }
  }, [connected]);

  const sendCommand = (cmd, payload) => {
    // In production, this sends over USB CDC-ACM or BLE
    // For now, log the command
    const frame = buildFrame(cmd, payload);
    console.log(`TX [${cmd.toString(16).padStart(2, '0')}]: ${Array.from(frame).map(b => b.toString(16).padStart(2, '0')).join(' ')}`);
  };

  const handleConnect = () => {
    if (connected) {
      sendCommand(CMD_DISCONNECT, null);
      setConnected(false);
    } else {
      sendCommand(CMD_CONNECT, null);
      setConnected(true);
    }
  };

  const handleStartMonitor = () => {
    sendCommand(CMD_SET_MODE, new Uint8Array([MODE_MONITOR]));
    navigation.navigate('Capture');
  };

  const handleStopMonitor = () => {
    sendCommand(CMD_SET_MODE, new Uint8Array([MODE_IDLE]));
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>WFP-100 WiFi Pentester</Text>

      <StatusCard
        connected={connected}
        mode={status.mode}
        channel={status.channel}
        band={status.band}
        batteryMv={status.batteryMv}
        packetsCaptured={status.packetsCaptured}
        packetsInjected={status.packetsInjected}
        temperature={status.temperature}
      />

      <TouchableOpacity
        style={[styles.button, connected ? styles.buttonDisconnect : styles.buttonConnect]}
        onPress={handleConnect}
      >
        <Text style={styles.buttonText}>
          {connected ? 'Disconnect' : 'Connect to WFP-100'}
        </Text>
      </TouchableOpacity>

      {connected && (
        <>
          <TouchableOpacity
            style={[styles.button, styles.buttonMonitor]}
            onPress={handleStartMonitor}
          >
            <Text style={styles.buttonText}>Start Monitor Mode</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[styles.button, styles.buttonStop]}
            onPress={handleStopMonitor}
          >
            <Text style={styles.buttonText}>Stop Capture</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[styles.button, styles.buttonSettings]}
            onPress={() => navigation.navigate('Settings')}
          >
            <Text style={styles.buttonText}>Channel & Band Settings</Text>
          </TouchableOpacity>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
  content: {
    padding: 16,
    alignItems: 'center',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#00ff88',
    marginBottom: 24,
    marginTop: 48,
  },
  button: {
    width: '100%',
    height: 48,
    borderRadius: 8,
    justifyContent: 'center',
    alignItems: 'center',
    marginTop: 12,
  },
  buttonConnect: {
    backgroundColor: '#00ff88',
  },
  buttonDisconnect: {
    backgroundColor: '#ff4444',
  },
  buttonMonitor: {
    backgroundColor: '#0ea5e9',
  },
  buttonStop: {
    backgroundColor: '#f97316',
  },
  buttonSettings: {
    backgroundColor: '#8b5cf6',
  },
  buttonText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#1a1a2e',
  },
});