/**
 * NFC Relay Phantom — Home Screen
 * Dashboard showing device status, battery, and quick actions
 */

import React, { useState, useEffect } from 'react';
import { View, ScrollView, StyleSheet } from 'react-native';
import { Text, Card, Button, ProgressBar, IconButton } from 'react-native-paper';
import { bleManager } from '../App';

export default function HomeScreen({ navigation }) {
  const [connected, setConnected] = useState(false);
  const [battery, setBattery] = useState(0);
  const [mode, setMode] = useState('IDLE');
  const [lastTag, setLastTag] = useState(null);

  useEffect(() => {
    const unsubscribe = bleManager.onConnectionChange((isConnected) => {
      setConnected(isConnected);
    });
    return unsubscribe;
  }, []);

  useEffect(() => {
    if (!connected) return;
    const interval = setInterval(async () => {
      const batt = await bleManager.readBattery();
      setBattery(batt);
    }, 10000);
    return () => clearInterval(interval);
  }, [connected]);

  const handleConnect = async () => {
    if (connected) {
      await bleManager.disconnect();
    } else {
      await bleManager.scanAndConnect();
    }
  };

  return (
    <ScrollView style={styles.container}>
      {/* Device Status Card */}
      <Card style={styles.card}>
        <Card.Title
          title="NFC Relay Phantom"
          subtitle={connected ? 'Connected' : 'Disconnected'}
          left={(props) => (
            <IconButton icon={connected ? 'bluetooth-connect' : 'bluetooth-off'} {...props} />
          )}
        />
        <Card.Content>
          <ProgressBar
            progress={battery / 100}
            color={battery > 20 ? '#4CAF50' : '#F44336'}
            style={styles.progressBar}
          />
          <Text style={styles.batteryText}>Battery: {battery}%</Text>
          <Text style={styles.modeText}>Mode: {mode}</Text>
        </Card.Content>
        <Card.Actions>
          <Button
            mode="contained"
            onPress={handleConnect}
            color={connected ? '#F44336' : '#6200EE'}
          >
            {connected ? 'Disconnect' : 'Connect'}
          </Button>
        </Card.Actions>
      </Card>

      {/* Quick Actions */}
      <Card style={styles.card}>
        <Card.Title title="Quick Actions" />
        <Card.Content>
          <View style={styles.actionGrid}>
            <Button
              mode="outlined"
              icon="nfc-variant"
              onPress={() => navigation.navigate('NFC', { mode: 'reader' })}
              style={styles.actionButton}
              disabled={!connected}
            >
              NFC Reader
            </Button>
            <Button
              mode="outlined"
              icon="nfc-variant"
              onPress={() => navigation.navigate('NFC', { mode: 'sniffer' })}
              style={styles.actionButton}
              disabled={!connected}
            >
              Sniffer
            </Button>
            <Button
              mode="outlined"
              icon="rfid"
              onPress={() => navigation.navigate('RFID125')}
              style={styles.actionButton}
              disabled={!connected}
            >
              125k RFID
            </Button>
            <Button
              mode="outlined"
              icon="swap-horizontal"
              onPress={() => navigation.navigate('Relay')}
              style={styles.actionButton}
              disabled={!connected}
            >
              Relay
            </Button>
          </View>
        </Card.Content>
      </Card>

      {/* Last Tag Info */}
      {lastTag && (
        <Card style={styles.card}>
          <Card.Title title="Last Tag Read" />
          <Card.Content>
            <Text style={styles.tagInfo}>
              Type: {lastTag.type}{'\n'}
              UID: {lastTag.uid}{'\n'}
              {lastTag.facilityCode && `Facility: ${lastTag.facilityCode}${'\n'}`}
              {lastTag.cardNumber && `Card #: ${lastTag.cardNumber}`}
            </Text>
          </Card.Content>
        </Card>
      )}

      {/* Warning */}
      <Card style={[styles.card, styles.warningCard]}>
        <Card.Content>
          <Text style={styles.warningText}>
            ⚠️ This device is for authorized security research only. Unauthorized
            interception or cloning of access credentials is illegal.
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
  progressBar: {
    height: 8,
    borderRadius: 4,
    marginVertical: 8,
  },
  batteryText: {
    fontSize: 14,
    color: '#666',
  },
  modeText: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#6200EE',
  },
  actionGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  actionButton: {
    width: '48%',
    marginVertical: 4,
  },
  tagInfo: {
    fontFamily: 'monospace',
    fontSize: 14,
  },
  warningCard: {
    backgroundColor: '#FFF3E0',
  },
  warningText: {
    color: '#E65100',
    fontSize: 12,
    textAlign: 'center',
  },
});