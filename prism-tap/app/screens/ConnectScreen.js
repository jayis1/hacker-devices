/**
 * screens/ConnectScreen.js — BLE Device Connection Screen
 *
 * Author: jayis1
 * License: GPL-2.0
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, FlatList, Alert } from 'react-native';
import { useDevice } from '../utils/deviceContext';

const ConnectScreen = ({ navigation }) => {
  const { scan, connect, disconnect, discoveredDevices, connected, connecting, error, device } = useDevice();
  const [autoScanned, setAutoScanned] = useState(false);

  useEffect(() => {
    if (!autoScanned && !connected) {
      scan();
      setAutoScanned(true);
    }
  }, [autoScanned, connected, scan]);

  const handleConnect = async (deviceId) => {
    await connect(deviceId);
    if (connected) {
      navigation.navigate('Capture');
    }
  };

  const handleDisconnect = async () => {
    Alert.alert(
      'Disconnect',
      'Disconnect from Prism-Tap?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Disconnect', style: 'destructive', onPress: () => disconnect() },
      ]
    );
  };

  const renderDevice = ({ item }) => (
    <TouchableOpacity style={styles.deviceItem} onPress={() => handleConnect(item.id)}>
      <Text style={styles.deviceName}>{item.name || 'Unknown'}</Text>
      <Text style={styles.deviceId}>{item.id}</Text>
      <Text style={styles.deviceRssi}>RSSI: {item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Prism-Tap</Text>
        <Text style={styles.subtitle}>MIPI CSI-2 / DSI Interface Tap</Text>
      </View>

      {connected && (
        <View style={styles.connectedBanner}>
          <Text style={styles.connectedText}>✓ Connected to {device?.name || 'Prism-Tap'}</Text>
          <View style={styles.connectedActions}>
            <TouchableOpacity
              style={styles.navButton}
              onPress={() => navigation.navigate('Capture')}
            >
              <Text style={styles.navButtonText}>Capture</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.navButton}
              onPress={() => navigation.navigate('Inject')}
            >
              <Text style={styles.navButtonText}>Inject</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.navButton}
              onPress={() => navigation.navigate('Gallery')}
            >
              <Text style={styles.navButtonText}>Gallery</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.navButton}
              onPress={() => navigation.navigate('Settings')}
            >
              <Text style={styles.navButtonText}>Settings</Text>
            </TouchableOpacity>
          </View>
          <TouchableOpacity style={styles.disconnectButton} onPress={handleDisconnect}>
            <Text style={styles.disconnectText}>Disconnect</Text>
          </TouchableOpacity>
        </View>
      )}

      {!connected && (
        <View style={styles.scanSection}>
          <Text style={styles.sectionTitle}>
            {connecting ? 'Scanning...' : 'Discovered Devices'}
          </Text>

          {error && (
            <Text style={styles.errorText}>Error: {error}</Text>
          )}

          <FlatList
            data={discoveredDevices}
            keyExtractor={(item) => item.id}
            renderItem={renderDevice}
            ListEmptyComponent={
              <Text style={styles.emptyText}>
                No devices found. Make sure Prism-Tap is powered on and in range.
              </Text>
            }
            style={styles.deviceList}
          />

          <TouchableOpacity style={styles.scanButton} onPress={scan}>
            <Text style={styles.scanButtonText}>Rescan</Text>
          </TouchableOpacity>
        </View>
      )}

      <View style={styles.footer}>
        <Text style={styles.footerText}>Author: jayis1 | v1.0 | GPL-2.0</Text>
        <Text style={styles.disclaimer}>
          For authorized security research only. Use only on devices you own
          or have explicit written permission to test.
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1e',
  },
  header: {
    padding: 20,
    alignItems: 'center',
    borderBottomWidth: 1,
    borderBottomColor: '#1a1a2e',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#00d9ff',
  },
  subtitle: {
    fontSize: 12,
    color: '#666',
    marginTop: 4,
  },
  connectedBanner: {
    padding: 15,
    backgroundColor: '#1a2e1a',
    borderBottomWidth: 1,
    borderBottomColor: '#2a4a2a',
  },
  connectedText: {
    color: '#4caf50',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 10,
  },
  connectedActions: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 10,
  },
  navButton: {
    backgroundColor: '#1a1a2e',
    padding: 8,
    borderRadius: 5,
    borderWidth: 1,
    borderColor: '#00d9ff',
  },
  navButtonText: {
    color: '#00d9ff',
    fontSize: 12,
  },
  disconnectButton: {
    backgroundColor: '#2e1a1a',
    padding: 8,
    borderRadius: 5,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#ff4444',
  },
  disconnectText: {
    color: '#ff4444',
    fontSize: 12,
  },
  scanSection: {
    flex: 1,
    padding: 15,
  },
  sectionTitle: {
    color: '#00d9ff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 10,
  },
  deviceList: {
    flex: 1,
  },
  deviceItem: {
    backgroundColor: '#1a1a2e',
    padding: 15,
    borderRadius: 8,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  deviceName: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  deviceId: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
  },
  deviceRssi: {
    color: '#666',
    fontSize: 11,
    marginTop: 2,
  },
  errorText: {
    color: '#ff4444',
    fontSize: 12,
    marginBottom: 10,
  },
  emptyText: {
    color: '#666',
    textAlign: 'center',
    marginTop: 30,
    fontSize: 14,
  },
  scanButton: {
    backgroundColor: '#00d9ff',
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 10,
  },
  scanButtonText: {
    color: '#0f0f1e',
    fontWeight: 'bold',
    fontSize: 14,
  },
  footer: {
    padding: 15,
    borderTopWidth: 1,
    borderTopColor: '#1a1a2e',
  },
  footerText: {
    color: '#444',
    fontSize: 10,
    textAlign: 'center',
  },
  disclaimer: {
    color: '#553333',
    fontSize: 9,
    textAlign: 'center',
    marginTop: 4,
  },
});

export default ConnectScreen;