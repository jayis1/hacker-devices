/**
 * DiscoveryScreen.js — SATA Phantom Device Discovery
 * Author: jayis1
 *
 * Scans for SATA Phantom devices on the local network using mDNS/BLE.
 * Displays discovered devices with status, battery level, and connection actions.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet,
  ActivityIndicator, RefreshControl, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { discoverDevices } from '../services/mdns';

const DiscoveryScreen = () => {
  const [devices, setDevices] = useState([]);
  const [scanning, setScanning] = useState(false);
  const [connectedDevice, setConnectedDevice] = useState(null);

  const startScan = useCallback(async () => {
    setScanning(true);
    try {
      const found = await discoverDevices();
      setDevices(found);
    } catch (error) {
      console.error('Discovery error:', error);
    } finally {
      setScanning(false);
    }
  }, []);

  useEffect(() => {
    startScan();
  }, [startScan]);

  const handleConnect = (device) => {
    setConnectedDevice(device);
    Alert.alert('Connected', `Connected to ${device.name} at ${device.ip}`);
  };

  const handleDisconnect = () => {
    setConnectedDevice(null);
    Alert.alert('Disconnected', 'Device disconnected');
  };

  const getModeColor = (mode) => {
    switch (mode) {
      case 0: return '#4a4';  // Transparent - green
      case 1: return '#48f';  // Monitor - blue
      case 2: return '#f44';  // Active - red
      case 3: return '#fa4';  // Exfil - orange
      case 4: return '#888';  // Sleep - gray
      case 5: return '#f0f';  // USB Config - magenta
      default: return '#888';
    }
  };

  const getModeName = (mode) => {
    const modes = ['Transparent', 'Monitor', 'Active', 'Exfil', 'Sleep', 'USB Config'];
    return modes[mode] || 'Unknown';
  };

  const renderDeviceItem = ({ item }) => (
    <TouchableOpacity
      style={[styles.deviceCard, connectedDevice?.id === item.id && styles.connectedCard]}
      onPress={() => handleConnect(item)}
      onLongPress={() => {
        Alert.alert(item.name, `IP: ${item.ip}\nMAC: ${item.mac}\nFirmware: ${item.firmware}`);
      }}
    >
      <View style={styles.deviceHeader}>
        <Icon name="chip" size={28} color="#00ff88" />
        <View style={styles.deviceInfo}>
          <Text style={styles.deviceName}>{item.name || 'SATA Phantom'}</Text>
          <Text style={styles.deviceIP}>{item.ip}:{item.port}</Text>
        </View>
        <View style={[styles.modeIndicator, { backgroundColor: getModeColor(item.mode) }]}>
          <Text style={styles.modeText}>{getModeName(item.mode).substring(0, 4)}</Text>
        </View>
      </View>

      <View style={styles.deviceDetails}>
        <View style={styles.detailRow}>
          <Icon name="signal" size={14} color="#aaa" />
          <Text style={styles.detailText}>
            Link: {item.linkUp ? 'UP @ ' + (item.speed === 2 ? '6.0G' : item.speed === 1 ? '3.0G' : '1.5G') : 'DOWN'}
          </Text>
        </View>
        <View style={styles.detailRow}>
          <Icon name="battery" size={14} color={item.battery > 3500 ? '#4a4' : '#f44'} />
          <Text style={styles.detailText}>Battery: {item.battery} mV</Text>
        </View>
        <View style={styles.detailRow}>
          <Icon name="counter" size={14} color="#aaa" />
          <Text style={styles.detailText}>Frames: {item.framesCaptured}</Text>
        </View>
        <View style={styles.detailRow}>
          <Icon name="filter-variant" size={14} color="#aaa" />
          <Text style={styles.detailText}>Rules: {item.rulesActive} active</Text>
        </View>
      </View>

      {connectedDevice?.id === item.id && (
        <TouchableOpacity style={styles.disconnectBtn} onPress={handleDisconnect}>
          <Text style={styles.disconnectBtnText}>DISCONNECT</Text>
        </TouchableOpacity>
      )}
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>SATA Phantom</Text>
        <Text style={styles.subtitle}>by jayis1</Text>
      </View>

      {connectedDevice && (
        <View style={styles.connectedBar}>
          <Icon name="checkbox-marked-circle" size={16} color="#00ff88" />
          <Text style={styles.connectedText}>
            Connected to {connectedDevice.name}
          </Text>
        </View>
      )}

      <FlatList
        data={devices}
        renderItem={renderDeviceItem}
        keyExtractor={(item) => item.id}
        contentContainerStyle={styles.listContent}
        refreshControl={
          <RefreshControl
            refreshing={scanning}
            onRefresh={startScan}
            tintColor="#00ff88"
          />
        }
        ListEmptyComponent={
          <View style={styles.emptyState}>
            {scanning ? (
              <ActivityIndicator size="large" color="#00ff88" />
            ) : (
              <>
                <Icon name="radar-off" size={48} color="#555" />
                <Text style={styles.emptyText}>No SATA Phantom devices found</Text>
                <Text style={styles.emptySubtext}>
                  Ensure the device is powered and on the same network
                </Text>
                <TouchableOpacity style={styles.scanBtn} onPress={startScan}>
                  <Icon name="radar" size={20} color="#fff" />
                  <Text style={styles.scanBtnText}>Scan Again</Text>
                </TouchableOpacity>
              </>
            )}
          </View>
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  header: {
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a4a',
  },
  title: {
    fontSize: 22,
    fontWeight: 'bold',
    color: '#00ff88',
  },
  subtitle: {
    fontSize: 12,
    color: '#666',
    marginTop: 2,
  },
  connectedBar: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#0a2a1a',
    padding: 8,
    paddingHorizontal: 16,
  },
  connectedText: {
    color: '#00ff88',
    marginLeft: 8,
    fontSize: 13,
  },
  listContent: {
    padding: 12,
  },
  deviceCard: {
    backgroundColor: '#12122a',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#2a2a4a',
  },
  connectedCard: {
    borderColor: '#00ff88',
    borderWidth: 2,
  },
  deviceHeader: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  deviceInfo: {
    flex: 1,
    marginLeft: 12,
  },
  deviceName: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
  },
  deviceIP: {
    fontSize: 12,
    color: '#888',
    marginTop: 2,
  },
  modeIndicator: {
    borderRadius: 6,
    paddingHorizontal: 8,
    paddingVertical: 4,
  },
  modeText: {
    color: '#fff',
    fontSize: 11,
    fontWeight: 'bold',
  },
  deviceDetails: {
    marginTop: 12,
    borderTopWidth: 1,
    borderTopColor: '#2a2a4a',
    paddingTop: 10,
  },
  detailRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 6,
  },
  detailText: {
    color: '#aaa',
    fontSize: 13,
    marginLeft: 8,
  },
  disconnectBtn: {
    marginTop: 10,
    backgroundColor: '#441111',
    borderRadius: 6,
    paddingVertical: 8,
    alignItems: 'center',
  },
  disconnectBtnText: {
    color: '#f44',
    fontWeight: 'bold',
    fontSize: 13,
  },
  emptyState: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 60,
  },
  emptyText: {
    color: '#888',
    fontSize: 16,
    marginTop: 16,
  },
  emptySubtext: {
    color: '#555',
    fontSize: 12,
    marginTop: 8,
    textAlign: 'center',
    paddingHorizontal: 40,
  },
  scanBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1a3a2a',
    borderRadius: 8,
    paddingHorizontal: 20,
    paddingVertical: 12,
    marginTop: 20,
  },
  scanBtnText: {
    color: '#fff',
    marginLeft: 8,
    fontWeight: 'bold',
  },
});

export default DiscoveryScreen;
