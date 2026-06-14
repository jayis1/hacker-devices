/**
 * PHANTOM — Device Connection & Status Screen
 * Manages BLE connection to PHANTOM hardware
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
  Vibration,
} from 'react-native';
import { BleContext } from '../components/BleContext';

const DeviceScreen = ({ navigation }) => {
  const {
    devices,
    connectedDevice,
    connect,
    disconnect,
    scan,
    isScanning,
    deviceInfo,
    batteryLevel,
    deviceState,
  } = useContext(BleContext);

  const [autoReconnect, setAutoReconnect] = useState(true);

  useEffect(() => {
    // Start scanning on mount
    scan();
  }, []);

  const handleConnect = async (device) => {
    try {
      Vibration.vibrate(50);
      await connect(device);
      Alert.alert('Connected', `Connected to ${device.name || device.id}`);
    } catch (error) {
      Alert.alert('Connection Failed', error.message);
    }
  };

  const handleDisconnect = async () => {
    try {
      await disconnect();
      Alert.alert('Disconnected', 'Device disconnected');
    } catch (error) {
      Alert.alert('Error', error.message);
    }
  };

  const getStateColor = () => {
    switch (deviceState) {
      case 'STEALTH': return '#3498db';
      case 'HID': return '#2ecc71';
      case 'EXECUTING': return '#e74c3c';
      case 'GEOFENCE': return '#f39c12';
      case 'KILL': return '#95a5a6';
      case 'ERROR': return '#e74c3c';
      default: return '#666';
    }
  };

  const getStateIcon = () => {
    switch (deviceState) {
      case 'STEALTH': return '⚡';
      case 'HID': return '⌨️';
      case 'EXECUTING': return '🚀';
      case 'GEOFENCE': return '📍';
      case 'KILL': return '🔴';
      case 'ERROR': return '❌';
      default: return '❓';
    }
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection Status Card */}
      <View style={[styles.card, styles.statusCard]}>
        <Text style={styles.cardTitle}>DEVICE STATUS</Text>

        {connectedDevice ? (
          <View>
            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>State</Text>
              <View style={[styles.stateBadge, { backgroundColor: getStateColor() }]}>
                <Text style={styles.stateText}>
                  {getStateIcon()} {deviceState || 'UNKNOWN'}
                </Text>
              </View>
            </View>

            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>Device</Text>
              <Text style={styles.statusValue}>
                {deviceInfo?.name || connectedDevice.name || 'PHANTOM'}
              </Text>
            </View>

            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>Battery</Text>
              <View style={styles.batteryContainer}>
                <View style={[styles.batteryBar, { width: `${batteryLevel}%` }]} />
                <Text style={styles.batteryText}>{batteryLevel}%</Text>
              </View>
            </View>

            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>Firmware</Text>
              <Text style={styles.statusValue}>
                v{deviceInfo?.firmwareVersion || '1.0.0'}
              </Text>
            </View>

            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>Profiles</Text>
              <Text style={styles.statusValue}>
                {deviceInfo?.profileCount || 0} loaded
              </Text>
            </View>

            <TouchableOpacity
              style={[styles.button, styles.disconnectButton]}
              onPress={handleDisconnect}
            >
              <Text style={styles.buttonText}>Disconnect</Text>
            </TouchableOpacity>
          </View>
        ) : (
          <View style={styles.disconnectedContainer}>
            <Text style={styles.disconnectedText}>No Device Connected</Text>
            <Text style={styles.disconnectedSubtext}>
              Scan for PHANTOM devices to connect
            </Text>
          </View>
        )}
      </View>

      {/* Quick Actions */}
      {connectedDevice && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>QUICK ACTIONS</Text>

          <View style={styles.actionGrid}>
            <TouchableOpacity
              style={[styles.actionButton, { borderColor: '#2ecc71' }]}
              onPress={() => navigation.navigate('Payloads')}
            >
              <Text style={styles.actionIcon}>📝</Text>
              <Text style={styles.actionText}>Payloads</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={[styles.actionButton, { borderColor: '#3498db' }]}
              onPress={() => navigation.navigate('Settings')}
            >
              <Text style={styles.actionIcon}>⚙️</Text>
              <Text style={styles.actionText}>Settings</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={[styles.actionButton, { borderColor: '#e74c3c' }]}
              onPress={() => {
                Alert.alert(
                  'Execute',
                  'This will execute the selected payload on the target device. Continue?',
                  [
                    { text: 'Cancel', style: 'cancel' },
                    {
                      text: 'Execute',
                      style: 'destructive',
                      onPress: () => {
                        Vibration.vibrate(200);
                        // Send execute command via BLE
                      },
                    },
                  ]
                );
              }}
            >
              <Text style={styles.actionIcon}>🚀</Text>
              <Text style={styles.actionText}>Execute</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={[styles.actionButton, { borderColor: '#9b59b6' }]}
              onPress={() => {
                // Switch USB mode
                Alert.alert('Switch Mode', 'Toggle between HID and Mass Storage mode?');
              }}
            >
              <Text style={styles.actionIcon}>🔄</Text>
              <Text style={styles.actionText}>Switch Mode</Text>
            </TouchableOpacity>
          </View>
        </View>
      )}

      {/* Device List */}
      {!connectedDevice && (
        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardTitle}>AVAILABLE DEVICES</Text>
            <TouchableOpacity style={styles.scanButton} onPress={scan}>
              <Text style={styles.scanButtonText}>
                {isScanning ? 'Scanning...' : 'Scan'}
              </Text>
            </TouchableOpacity>
          </View>

          {devices.length === 0 ? (
            <View style={styles.emptyContainer}>
              <Text style={styles.emptyText}>
                {isScanning ? 'Scanning for devices...' : 'No devices found'}
              </Text>
              <Text style={styles.emptySubtext}>
                Make sure PHANTOM is powered on and within range
              </Text>
            </View>
          ) : (
            devices.map((device) => (
              <TouchableOpacity
                key={device.id}
                style={styles.deviceItem}
                onPress={() => handleConnect(device)}
              >
                <View style={styles.deviceInfo}>
                  <Text style={styles.deviceName}>
                    {device.name || 'Unknown Device'}
                  </Text>
                  <Text style={styles.deviceId}>{device.id}</Text>
                  <Text style={styles.deviceRssi}>
                    RSSI: {device.rssi || 'N/A'} dBm
                  </Text>
                </View>
                <Text style={styles.deviceConnect}>→</Text>
              </TouchableOpacity>
            ))
          )}
        </View>
      )}

      {/* Safety Notice */}
      <View style={styles.card}>
        <Text style={styles.disclaimerTitle}>⚠️ LEGAL NOTICE</Text>
        <Text style={styles.disclaimerText}>
          This tool is designed for authorized security testing only.
          Unauthorized access to computer systems is illegal. Always obtain
          proper written authorization before testing. The user assumes all
          responsibility for compliance with applicable laws.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  statusCard: {
    borderLeftWidth: 4,
    borderLeftColor: '#00ff41',
  },
  cardTitle: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 12,
    letterSpacing: 1,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  statusLabel: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 13,
  },
  statusValue: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 13,
  },
  stateBadge: {
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderRadius: 12,
  },
  stateText: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 12,
    fontWeight: 'bold',
  },
  batteryContainer: {
    flex: 1,
    maxWidth: 120,
    height: 20,
    backgroundColor: '#2d2d44',
    borderRadius: 10,
    overflow: 'hidden',
    position: 'relative',
  },
  batteryBar: {
    height: '100%',
    backgroundColor: '#2ecc71',
    borderRadius: 10,
  },
  batteryText: {
    position: 'absolute',
    right: 8,
    top: 2,
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 10,
  },
  disconnectedContainer: {
    alignItems: 'center',
    paddingVertical: 24,
  },
  disconnectedText: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 16,
    fontWeight: 'bold',
  },
  disconnectedSubtext: {
    color: '#444',
    fontFamily: 'monospace',
    fontSize: 12,
    marginTop: 8,
  },
  actionGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  actionButton: {
    width: '48%',
    aspectRatio: 1.2,
    backgroundColor: '#0f0f23',
    borderRadius: 12,
    borderWidth: 1,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 8,
  },
  actionIcon: {
    fontSize: 28,
    marginBottom: 4,
  },
  actionText: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 12,
  },
  button: {
    backgroundColor: '#2d2d44',
    borderRadius: 8,
    padding: 12,
    marginTop: 12,
    alignItems: 'center',
  },
  disconnectButton: {
    backgroundColor: '#3d1515',
    borderWidth: 1,
    borderColor: '#e74c3c',
  },
  buttonText: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
  scanButton: {
    backgroundColor: '#00ff41',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 4,
  },
  scanButtonText: {
    color: '#0f0f23',
    fontFamily: 'monospace',
    fontSize: 11,
    fontWeight: 'bold',
  },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 12,
    paddingHorizontal: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
  deviceId: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 10,
    marginTop: 2,
  },
  deviceRssi: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 11,
    marginTop: 2,
  },
  deviceConnect: {
    color: '#00ff41',
    fontSize: 24,
    fontFamily: 'monospace',
  },
  emptyContainer: {
    alignItems: 'center',
    paddingVertical: 24,
  },
  emptyText: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 14,
  },
  emptySubtext: {
    color: '#555',
    fontFamily: 'monospace',
    fontSize: 11,
    marginTop: 8,
    textAlign: 'center',
  },
  disclaimerTitle: {
    color: '#f39c12',
    fontFamily: 'monospace',
    fontSize: 12,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  disclaimerText: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 10,
    lineHeight: 16,
  },
});

export default DeviceScreen;