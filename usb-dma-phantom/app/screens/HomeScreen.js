/**
 * HomeScreen — Dashboard showing device status and quick actions
 * USB DMA Phantom Companion App
 */

import React, { useContext, useState, useEffect } from 'react';
import { View, StyleSheet, ScrollView, Alert } from 'react-native';
import { Text, Card, Button, IconButton, Badge, ProgressBar } from 'react-native-paper';
import { BleContext } from '../utils/protocol';

const STATUS_COLORS = {
  disconnected: '#666666',
  connecting: '#ffaa00',
  connected: '#00ff88',
  dma_active: '#ff3366',
  error: '#ff0000',
};

export default function HomeScreen({ navigation }) {
  const { device, connectionStatus, dmaStatus, connect, disconnect } = useContext(BleContext);
  const [batteryLevel, setBatteryLevel] = useState(null);
  const [linkStatus, setLinkStatus] = useState('unknown');

  useEffect(() => {
    if (connectionStatus === 'connected') {
      // Start polling device status
      const interval = setInterval(() => {
        // Would request status from device via BLE
      }, 2000);
      return () => clearInterval(interval);
    }
  }, [connectionStatus]);

  const statusColor = STATUS_COLORS[connectionStatus] || '#666666';
  const isConnected = connectionStatus === 'connected';

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Device Status Card */}
      <Card style={styles.card}>
        <Card.Title
          title="Device Status"
          left={(props) => <IconButton {...props} icon="usb" />}
          right={(props) => (
            <Badge
              size={12}
              style={[styles.statusDot, { backgroundColor: statusColor }]}
            />
          )}
        />
        <Card.Content>
          <View style={styles.statusRow}>
            <Text style={styles.label}>Connection</Text>
            <Text style={[styles.value, { color: statusColor }]}>
              {connectionStatus.toUpperCase()}
            </Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.label}>PCIe Link</Text>
            <Text style={styles.value}>
              {isConnected ? (linkStatus === 'up' ? 'UP' : 'DOWN') : '—'}
            </Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.label}>DMA Engine</Text>
            <Text style={[styles.value, { color: dmaStatus?.active ? '#ff3366' : '#00ff88' }]}>
              {isConnected ? (dmaStatus?.active ? 'ACTIVE' : 'IDLE') : '—'}
            </Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.label}>Mode</Text>
            <Text style={styles.value}>
              {isConnected ? (dmaStatus?.mode || 'STEALTH') : '—'}
            </Text>
          </View>
        </Card.Content>
      </Card>

      {/* Quick Actions */}
      <Card style={styles.card}>
        <Card.Title title="Quick Actions" />
        <Card.Content>
          <View style={styles.buttonRow}>
            <Button
              mode="contained"
              icon="connection"
              onPress={() => isConnected ? disconnect() : connect()}
              style={[styles.actionButton, { backgroundColor: isConnected ? '#ff3366' : '#00ff88' }]}
              labelStyle={styles.buttonLabel}
            >
              {isConnected ? 'Disconnect' : 'Connect'}
            </Button>
          </View>
          <View style={styles.buttonRow}>
            <Button
              mode="outlined"
              icon="memory"
              onPress={() => navigation.navigate('DmaControl')}
              style={styles.actionButton}
              disabled={!isConnected}
              labelStyle={styles.buttonLabel}
            >
              DMA Control
            </Button>
            <Button
              mode="outlined"
              icon="package-variant"
              onPress={() => navigation.navigate('PayloadManager')}
              style={styles.actionButton}
              disabled={!isConnected}
              labelStyle={styles.buttonLabel}
            >
              Payloads
            </Button>
          </View>
          <View style={styles.buttonRow}>
            <Button
              mode="outlined"
              icon="cog"
              onPress={() => navigation.navigate('Settings')}
              style={styles.actionButton}
              labelStyle={styles.buttonLabel}
            >
              Settings
            </Button>
          </View>
        </Card.Content>
      </Card>

      {/* Safety Warning */}
      <Card style={[styles.card, styles.warningCard]}>
        <Card.Content>
          <Text style={styles.warningText}>
            ⚠️ This device is for AUTHORIZED security research only.
            Unauthorized use against systems you do not own or have
            explicit permission to test is ILLEGAL. Always follow
            responsible disclosure practices.
          </Text>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a0a',
  },
  content: {
    padding: 16,
  },
  card: {
    backgroundColor: '#1a1a2e',
    marginBottom: 16,
    borderRadius: 12,
  },
  warningCard: {
    backgroundColor: '#2a1a1a',
    borderColor: '#ff3366',
    borderWidth: 1,
  },
  warningText: {
    color: '#ff8866',
    fontSize: 12,
    textAlign: 'center',
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  label: {
    color: '#888888',
    fontSize: 14,
  },
  value: {
    color: '#e0e0e0',
    fontSize: 14,
    fontWeight: 'bold',
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginVertical: 6,
  },
  actionButton: {
    flex: 1,
    marginHorizontal: 4,
    borderColor: '#00ff88',
    borderRadius: 8,
  },
  buttonLabel: {
    fontSize: 12,
  },
  statusDot: {
    alignSelf: 'center',
  },
});