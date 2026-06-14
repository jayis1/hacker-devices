/**
 * DeviceScreen.js — Device Connection & Status
 * Shows BLE connection state, device info, battery, and firmware version
 */

import React, { useContext, useEffect, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  RefreshControl,
} from 'react-native';
import { BleContext } from '../components/BleContext';
import StatusCard from '../components/StatusCard';

export default function DeviceScreen({ navigation }) {
  const {
    device,
    connectionState,
    connect,
    disconnect,
    scanForDevices,
    deviceInfo,
  } = useContext(BleContext);

  const [refreshing, setRefreshing] = useState(false);
  const [scanResults, setScanResults] = useState([]);

  const onRefresh = async () => {
    setRefreshing(true);
    const results = await scanForDevices();
    setScanResults(results);
    setRefreshing(false);
  };

  const getStatusColor = () => {
    switch (connectionState) {
      case 'connected': return '#00E676';
      case 'connecting': return '#FFC107';
      case 'disconnected': return '#F44336';
      default: return '#757575';
    }
  };

  const getStatusText = () => {
    switch (connectionState) {
      case 'connected': return 'Connected';
      case 'connecting': return 'Connecting...';
      case 'disconnected': return 'Disconnected';
      default: return 'Unknown';
    }
  };

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} />
      }
    >
      {/* Connection Status Card */}
      <StatusCard
        title="Device Status"
        icon="access-point"
        color={getStatusColor()}
      >
        <View style={styles.statusRow}>
          <Text style={styles.statusLabel}>Connection</Text>
          <Text style={[styles.statusValue, { color: getStatusColor() }]}>
            {getStatusText()}
          </Text>
        </View>
        {device && (
          <>
            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>Device Name</Text>
              <Text style={styles.statusValue}>{device.name || 'Unknown'}</Text>
            </View>
            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>MAC Address</Text>
              <Text style={styles.statusValue}>{device.id || 'N/A'}</Text>
            </View>
            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>RSSI</Text>
              <Text style={styles.statusValue}>{device.rssi || 'N/A'} dBm</Text>
            </View>
          </>
        )}
      </StatusCard>

      {/* Device Info Card */}
      {connectionState === 'connected' && deviceInfo && (
        <StatusCard title="Device Info" icon="information" color="#2196F3">
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Firmware</Text>
            <Text style={styles.statusValue}>{deviceInfo.firmwareVersion}</Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Mode</Text>
            <Text style={styles.statusValue}>{deviceInfo.mode}</Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Frequency</Text>
            <Text style={styles.statusValue}>
              {(deviceInfo.frequency / 1e6).toFixed(1)} MHz
            </Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Packets Captured</Text>
            <Text style={styles.statusValue}>{deviceInfo.packetCount}</Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Uptime</Text>
            <Text style={styles.statusValue}>
              {Math.floor(deviceInfo.uptime / 60)}m {deviceInfo.uptime % 60}s
            </Text>
          </View>
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Battery</Text>
            <Text style={styles.statusValue}>{deviceInfo.battery}%</Text>
          </View>
        </StatusCard>
      )}

      {/* Scan Results */}
      <StatusCard title="Available Devices" icon="bluetooth" color="#9C27B0">
        {scanResults.length === 0 ? (
          <Text style={styles.emptyText}>Pull down to scan for devices</Text>
        ) : (
          scanResults.map((dev, index) => (
            <TouchableOpacity
              key={index}
              style={styles.deviceItem}
              onPress={() => connect(dev.id)}
            >
              <Text style={styles.deviceName}>{dev.name || 'Unknown'}</Text>
              <Text style={styles.deviceId}>{dev.id}</Text>
              <Text style={styles.deviceRssi}>{dev.rssi} dBm</Text>
            </TouchableOpacity>
          ))
        )}
      </StatusCard>

      {/* Connect/Disconnect Button */}
      <TouchableOpacity
        style={[
          styles.actionButton,
          connectionState === 'connected'
            ? styles.disconnectButton
            : styles.connectButton,
        ]}
        onPress={() => {
          if (connectionState === 'connected') {
            disconnect();
          } else {
            scanForDevices();
          }
        }}
      >
        <Text style={styles.actionButtonText}>
          {connectionState === 'connected' ? 'Disconnect' : 'Scan & Connect'}
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F23',
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A3E',
  },
  statusLabel: {
    color: '#B0BEC5',
    fontSize: 14,
  },
  statusValue: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
  emptyText: {
    color: '#757575',
    fontSize: 14,
    textAlign: 'center',
    paddingVertical: 20,
  },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
    paddingHorizontal: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A3E',
  },
  deviceName: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
    flex: 2,
  },
  deviceId: {
    color: '#B0BEC5',
    fontSize: 11,
    flex: 2,
  },
  deviceRssi: {
    color: '#FFC107',
    fontSize: 12,
    flex: 1,
    textAlign: 'right',
  },
  actionButton: {
    margin: 16,
    paddingVertical: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  connectButton: {
    backgroundColor: '#00E676',
  },
  disconnectButton: {
    backgroundColor: '#F44336',
  },
  actionButtonText: {
    color: '#000000',
    fontSize: 16,
    fontWeight: '700',
  },
});