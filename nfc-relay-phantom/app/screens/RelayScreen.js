/**
 * NFC Relay Phantom — Relay Screen
 * BLE relay tunnel between two Phantom devices
 */

import React, { useState, useCallback, useEffect } from 'react';
import { View, ScrollView, StyleSheet } from 'react-native';
import { Text, Card, Button, ProgressBar } from 'react-native-paper';
import { bleManager } from '../App';

export default function RelayScreen() {
  const [relayActive, setRelayActive] = useState(false);
  const [relayMode, setRelayMode] = useState('card'); // 'card' or 'reader'
  const [latency, setLatency] = useState(0);
  const [framesForwarded, setFramesForwarded] = useState(0);
  const [peerConnected, setPeerConnected] = useState(false);
  const [uptime, setUptime] = useState(0);

  useEffect(() => {
    if (!relayActive) return;
    const interval = setInterval(() => {
      setUptime(prev => prev + 1);
    }, 1000);
    return () => clearInterval(interval);
  }, [relayActive]);

  useEffect(() => {
    const unsub = bleManager.onRelayData((data) => {
      if (data.latency) setLatency(data.latency);
      if (data.frames) setFramesForwarded(data.frames);
      if (data.peerConnected !== undefined) setPeerConnected(data.peerConnected);
    });
    return unsub;
  }, []);

  const startRelay = useCallback(async () => {
    setRelayActive(true);
    setUptime(0);
    setFramesForwarded(0);
    setLatency(0);
    await bleManager.sendCommand('RELAY', 'START', { mode: relayMode });
  }, [relayMode]);

  const stopRelay = useCallback(async () => {
    setRelayActive(false);
    await bleManager.sendCommand('RELAY', 'STOP', {});
  }, []);

  const formatUptime = (seconds) => {
    const m = Math.floor(seconds / 60);
    const s = seconds % 60;
    return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  };

  return (
    <ScrollView style={styles.container}>
      {/* Relay Status */}
      <Card style={[styles.card, relayActive ? styles.activeCard : styles.inactiveCard]}>
        <Card.Title
          title={relayActive ? 'Relay Active' : 'Relay Inactive'}
          subtitle={relayMode === 'card' ? 'Prox Card Mode' : 'Prox Reader Mode'}
        />
        <Card.Content>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Status:</Text>
            <Text style={[styles.statusValue, { color: relayActive ? '#4CAF50' : '#999' }]}>
              {relayActive ? '● Running' : '○ Stopped'}
            </Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Peer:</Text>
            <Text style={[styles.statusValue, { color: peerConnected ? '#4CAF50' : '#F44336' }]}>
              {peerConnected ? '● Connected' : '○ Disconnected'}
            </Text>
          </View>
          {relayActive && (
            <>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Uptime:</Text>
                <Text style={styles.statusValue}>{formatUptime(uptime)}</Text>
              </View>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Latency:</Text>
                <Text style={styles.statusValue}>{latency} ms</Text>
              </View>
              <View style={styles.statusRow}>
                <Text style={styles.statusLabel}>Frames:</Text>
                <Text style={styles.statusValue}>{framesForwarded}</Text>
              </View>
            </>
          )}
        </Card.Content>
        <Card.Actions>
          <Button
            mode="contained"
            onPress={relayActive ? stopRelay : startRelay}
            color={relayActive ? '#F44336' : '#6200EE'}
          >
            {relayActive ? 'Stop Relay' : 'Start Relay'}
          </Button>
        </Card.Actions>
      </Card>

      {/* Mode Selection */}
      {!relayActive && (
        <Card style={styles.card}>
          <Card.Title title="Relay Mode" />
          <Card.Content>
            <Button
              mode={relayMode === 'card' ? 'contained' : 'outlined'}
              onPress={() => setRelayMode('card')}
              style={styles.modeButton}
              icon="nfc-variant"
            >
              Prox Card — Present to real reader
            </Button>
            <Button
              mode={relayMode === 'reader' ? 'contained' : 'outlined'}
              onPress={() => setRelayMode('reader')}
              style={styles.modeButton}
              icon="credit-card-reader"
            >
              Prox Reader — Read real card
            </Button>
          </Card.Content>
        </Card>
      )}

      {/* How It Works */}
      <Card style={styles.card}>
        <Card.Title title="How Relay Works" />
        <Card.Content>
          <Text style={styles.infoText}>
            {'Relay attack requires two Phantom devices:\n\n' +
             '1. Device A (Prox Card) is presented to the real reader\n' +
             '2. Device B (Prox Reader) reads the real card\n' +
             '3. Frames are tunneled over BLE between devices\n' +
             '4. Target latency: < 50ms round-trip\n\n' +
             '⚠️ For authorized security research demonstrations only.'}
          </Text>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  card: {
    margin: 8,
  },
  activeCard: {
    borderLeftWidth: 4,
    borderLeftColor: '#4CAF50',
  },
  inactiveCard: {
    borderLeftWidth: 4,
    borderLeftColor: '#999',
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  statusLabel: {
    fontSize: 14,
    color: '#666',
  },
  statusValue: {
    fontSize: 14,
    fontWeight: 'bold',
  },
  modeButton: {
    marginVertical: 4,
  },
  infoText: {
    fontSize: 13,
    lineHeight: 20,
    color: '#333',
  },
});